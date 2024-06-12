//$file${src::qf::qf_time.cpp} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
//
// Model: qpcpp.qm
// File:  ${src::qf::qf_time.cpp}
//
// This code has been generated by QM 6.1.1 <www.state-machine.com/qm>.
// DO NOT EDIT THIS FILE MANUALLY. All your changes will be lost.
//
// This code is covered by the following QP license:
// License #    : LicenseRef-QL-dual
// Issued to    : Any user of the QP/C++ real-time embedded framework
// Framework(s) : qpcpp
// Support ends : 2024-12-31
// License scope:
//
// Copyright (C) 2005 Quantum Leaps, LLC <state-machine.com>.
//
//                    Q u a n t u m  L e a P s
//                    ------------------------
//                    Modern Embedded Software
//
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-QL-commercial
//
// This software is dual-licensed under the terms of the open source GNU
// General Public License version 3 (or any later version), or alternatively,
// under the terms of one of the closed source Quantum Leaps commercial
// licenses.
//
// The terms of the open source GNU General Public License version 3
// can be found at: <www.gnu.org/licenses/gpl-3.0>
//
// The terms of the closed source Quantum Leaps commercial licenses
// can be found at: <www.state-machine.com/licensing>
//
// Redistributions in source code must retain this top-level comment block.
// Plagiarizing this software to sidestep the license obligations is illegal.
//
// Contact information:
// <www.state-machine.com/licensing>
// <info@state-machine.com>
//
//$endhead${src::qf::qf_time.cpp} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#define QP_IMPL             // this is QP implementation
#include "qp_port.hpp"      // QP port
#include "qp_pkg.hpp"       // QP package-scope interface
#include "qsafe.h"          // QP Functional Safety (FuSa) Subsystem
#ifdef Q_SPY                // QS software tracing enabled?
    #include "qs_port.hpp"  // QS port
    #include "qs_pkg.hpp"   // QS facilities for pre-defined trace records
#else
    #include "qs_dummy.hpp" // disable the QS software tracing
#endif // Q_SPY

// unnamed namespace for local definitions with internal linkage
namespace {
Q_THIS_MODULE("qf_time");
} // unnamed namespace

//$skip${QP_VERSION} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
// Check for the minimum required QP version
#if (QP_VERSION < 730U) || (QP_VERSION != ((QP_RELEASE^4294967295U) % 0x3E8U))
#error qpcpp version 7.3.0 or higher required
#endif
//$endskip${QP_VERSION} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

//$define${QF::QTimeEvt} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
namespace QP {

//${QF::QTimeEvt} ............................................................
QTimeEvt QTimeEvt::timeEvtHead_[QF_MAX_TICK_RATE];

//${QF::QTimeEvt::QTimeEvt} ..................................................
QTimeEvt::QTimeEvt(
    QActive * const act,
    QSignal const sig,
    std::uint_fast8_t const tickRate) noexcept
 :
    QEvt(sig),
    m_next(nullptr),
    m_act(act),
    m_ctr(0U),
    m_interval(0U)
{
    QF_CRIT_STAT
    QF_CRIT_ENTRY();
    Q_REQUIRE_INCRIT(300, (sig != 0U)
        && (tickRate < QF_MAX_TICK_RATE));
    QF_CRIT_EXIT();

    // The refCtr_ attribute is not used in time events, so it is
    // reused to hold the tickRate as well as other information
    refCtr_ = static_cast<std::uint8_t>(tickRate);
}

//${QF::QTimeEvt::armX} ......................................................
void QTimeEvt::armX(
    QTimeEvtCtr const nTicks,
    QTimeEvtCtr const interval) noexcept
{
    std::uint8_t const tickRate = refCtr_ & TE_TICK_RATE;
    QTimeEvtCtr const ctr = m_ctr;
    #ifdef Q_SPY
    std::uint_fast8_t const qsId =
         static_cast<QActive const *>(m_act)->m_prio;
    #endif

    QF_CRIT_STAT
    QF_CRIT_ENTRY();
    QF_MEM_SYS();

    Q_REQUIRE_INCRIT(400, (m_act != nullptr)
        && (ctr == 0U)
        && (nTicks != 0U)
        && (tickRate < static_cast<std::uint8_t>(QF_MAX_TICK_RATE))
        && (sig >= static_cast<QSignal>(Q_USER_SIG)));
    #ifdef Q_UNSAFE
    Q_UNUSED_PAR(ctr);
    #endif

    m_ctr = nTicks;
    m_interval = interval;

    // is the time event unlinked?
    // NOTE: For the duration of a single clock tick of the specified tick
    // rate a time event can be disarmed and yet still linked into the list
    // because un-linking is performed exclusively in the QF_tickX() function.
    if (static_cast<std::uint_fast8_t>(
           static_cast<std::uint_fast8_t>(refCtr_) & TE_IS_LINKED) == 0U)
    {
        // mark as linked
        refCtr_ = static_cast<std::uint8_t>(refCtr_ | TE_IS_LINKED);

        // The time event is initially inserted into the separate
        // "freshly armed" list based on timeEvtHead_[tickRate].act.
        // Only later, inside QTimeEvt::tick(), the "freshly armed"
        // list is appended to the main list of armed time events based on
        // timeEvtHead_[tickRate].next. Again, this is to keep any
        // changes to the main list exclusively inside QTimeEvt::tick().
        m_next = timeEvtHead_[tickRate].toTimeEvt();
        timeEvtHead_[tickRate].m_act = this;
    }

    QS_BEGIN_PRE_(QS_QF_TIMEEVT_ARM, qsId)
        QS_TIME_PRE_();        // timestamp
        QS_OBJ_PRE_(this);     // this time event object
        QS_OBJ_PRE_(m_act);    // the active object
        QS_TEC_PRE_(nTicks);   // the # ticks
        QS_TEC_PRE_(interval); // the interval
        QS_U8_PRE_(tickRate);  // tick rate
    QS_END_PRE_()

    QF_MEM_APP();
    QF_CRIT_EXIT();
}

//${QF::QTimeEvt::disarm} ....................................................
bool QTimeEvt::disarm() noexcept {
    #ifdef Q_SPY
    std::uint_fast8_t const qsId = static_cast<QActive *>(m_act)->m_prio;
    #endif

    QF_CRIT_STAT
    QF_CRIT_ENTRY();
    QF_MEM_SYS();

    // is the time event actually armed?
    bool wasArmed;
    if (m_ctr != 0U) {
        wasArmed = true;
        refCtr_ = static_cast<std::uint8_t>(refCtr_ | TE_WAS_DISARMED);

        QS_BEGIN_PRE_(QS_QF_TIMEEVT_DISARM, qsId)
            QS_TIME_PRE_();            // timestamp
            QS_OBJ_PRE_(this);         // this time event object
            QS_OBJ_PRE_(m_act);        // the target AO
            QS_TEC_PRE_(m_ctr);        // the # ticks
            QS_TEC_PRE_(m_interval);   // the interval
            QS_U8_PRE_(refCtr_& TE_TICK_RATE); // tick rate
        QS_END_PRE_()

        m_ctr = 0U; // schedule removal from the list
    }
    else { // the time event was already disarmed automatically
        wasArmed = false;
        refCtr_ = static_cast<std::uint8_t>(refCtr_
            & static_cast<std::uint8_t>(~TE_WAS_DISARMED));

        QS_BEGIN_PRE_(QS_QF_TIMEEVT_DISARM_ATTEMPT, qsId)
            QS_TIME_PRE_();            // timestamp
            QS_OBJ_PRE_(this);         // this time event object
            QS_OBJ_PRE_(m_act);        // the target AO
            QS_U8_PRE_(refCtr_& TE_TICK_RATE); // tick rate
        QS_END_PRE_()
    }

    QF_MEM_APP();
    QF_CRIT_EXIT();

    return wasArmed;
}

//${QF::QTimeEvt::rearm} .....................................................
bool QTimeEvt::rearm(QTimeEvtCtr const nTicks) noexcept {
    std::uint8_t const tickRate = refCtr_ & TE_TICK_RATE;

    #ifdef Q_SPY
    std::uint_fast8_t const qsId = static_cast<QActive *>(m_act)->m_prio;
    #endif

    QF_CRIT_STAT
    QF_CRIT_ENTRY();
    QF_MEM_SYS();

    Q_REQUIRE_INCRIT(600, (m_act != nullptr)
        && (tickRate < static_cast<std::uint8_t>(QF_MAX_TICK_RATE))
        && (nTicks != 0U)
        && (sig >= static_cast<QSignal>(Q_USER_SIG)));

    // is the time evt not running?
    bool wasArmed;
    if (m_ctr == 0U) {
        wasArmed = false;

        // NOTE: For a duration of a single clock tick of the specified
        // tick rate a time event can be disarmed and yet still linked into
        // the list, because unlinking is performed exclusively in the
        // QTimeEvt::tickX() function.

        // is the time event unlinked?
        if (static_cast<std::uint8_t>(refCtr_ & TE_IS_LINKED) == 0U) {
            // mark as linked
            refCtr_ = static_cast<std::uint8_t>(refCtr_ | TE_IS_LINKED);

            // The time event is initially inserted into the separate
            // "freshly armed" list based on timeEvtHead_[tickRate].act.
            // Only later, inside QTimeEvt::tick(), the "freshly armed"
            // list is appended to the main list of armed time events based on
            // timeEvtHead_[tickRate].next. Again, this is to keep any
            // changes to the main list exclusively inside QTimeEvt::tick().
            m_next = timeEvtHead_[tickRate].toTimeEvt();
            timeEvtHead_[tickRate].m_act = this;
        }
    }
    else { // the time event was armed
        wasArmed = true;
    }
    m_ctr = nTicks; // re-load the tick counter (shift the phasing)

    QS_BEGIN_PRE_(QS_QF_TIMEEVT_REARM, qsId)
        QS_TIME_PRE_();            // timestamp
        QS_OBJ_PRE_(this);         // this time event object
        QS_OBJ_PRE_(m_act);        // the target AO
        QS_TEC_PRE_(m_ctr);        // the # ticks
        QS_TEC_PRE_(m_interval);   // the interval
        QS_2U8_PRE_(tickRate, (wasArmed ? 1U : 0U));
    QS_END_PRE_()

    QF_MEM_APP();
    QF_CRIT_EXIT();

    return wasArmed;
}

//${QF::QTimeEvt::wasDisarmed} ...............................................
bool QTimeEvt::wasDisarmed() noexcept {
    QF_CRIT_STAT
    QF_CRIT_ENTRY();
    QF_MEM_SYS();

    std::uint8_t const isDisarmed = refCtr_ & TE_WAS_DISARMED;
    refCtr_ = static_cast<std::uint8_t>(refCtr_ | TE_WAS_DISARMED);

    QF_MEM_APP();
    QF_CRIT_EXIT();

    return isDisarmed != 0U;
}

//${QF::QTimeEvt::tick} ......................................................
void QTimeEvt::tick(
    std::uint_fast8_t const tickRate,
    void const * const sender) noexcept
{
    #ifndef Q_SPY
    Q_UNUSED_PAR(sender);
    #endif

    QF_CRIT_STAT
    QF_CRIT_ENTRY();
    QF_MEM_SYS();

    Q_REQUIRE_INCRIT(100, tickRate < Q_DIM(timeEvtHead_));

    QTimeEvt *prev = &timeEvtHead_[tickRate];

    QS_BEGIN_PRE_(QS_QF_TICK, 0U)
        prev->m_ctr = (prev->m_ctr + 1U);
        QS_TEC_PRE_(prev->m_ctr); // tick ctr
        QS_U8_PRE_(tickRate);     // tick rate
    QS_END_PRE_()

    // scan the linked-list of time events at this rate...
    std::uint_fast8_t lbound = 2U*QF_MAX_ACTIVE; // fixed upper loop bound
    for (; lbound > 0U; --lbound) {
        QTimeEvt *e = prev->m_next; // advance down the time evt. list

        if (e == nullptr) { // end of the list?

            // any new time events armed since the last run of tick()?
            if (timeEvtHead_[tickRate].m_act != nullptr) {

                // sanity check
                Q_ASSERT_INCRIT(110, prev != nullptr);
                prev->m_next = timeEvtHead_[tickRate].toTimeEvt();
                timeEvtHead_[tickRate].m_act = nullptr;
                e = prev->m_next; // switch to the new list
            }
            else { // all currently armed time events are processed
                break; // terminate the for-loop
            }
        }

        // the time event 'e' must be valid
        Q_INVARIANT_INCRIT(112, QEvt::verify_(e));

        if (e->m_ctr == 0U) { // time event scheduled for removal?
            prev->m_next = e->m_next;
            // mark time event 'e' as NOT linked
            e->refCtr_ = static_cast<std::uint8_t>(e->refCtr_
                & static_cast<std::uint8_t>(~TE_IS_LINKED));
            // do NOT advance the prev pointer
            QF_MEM_APP();
            QF_CRIT_EXIT(); // exit crit. section to reduce latency

            // NOTE: prevent merging critical sections
            // In some QF ports the critical section exit takes effect only
            // on the next machine instruction. If the next instruction is
            // another entry to a critical section, the critical section
            // might not be really exited, but rather the two adjacent
            // critical sections would be MERGED. The QF_CRIT_EXIT_NOP()
            // macro contains minimal code required to prevent such merging
            // of critical sections in QF ports, in which it can occur.
            QF_CRIT_EXIT_NOP();
        }
        else {
            e->m_ctr = (e->m_ctr - 1U);

            if (e->m_ctr == 0U) { // is time evt about to expire?
                QActive * const act = e->toActive();

                if (e->m_interval != 0U) { // periodic time evt?
                    e->m_ctr = e->m_interval; // rearm the time event
                    prev = e; // advance to this time event
                }
                else { // one-shot time event: automatically disarm
                    prev->m_next = e->m_next;

                    // mark time event 'e' as NOT linked
                    e->refCtr_ = static_cast<std::uint8_t>(e->refCtr_
                        & static_cast<std::uint8_t>(~TE_IS_LINKED));
                    // do NOT advance the prev pointer

                    QS_BEGIN_PRE_(QS_QF_TIMEEVT_AUTO_DISARM, act->m_prio)
                        QS_OBJ_PRE_(e);        // this time event object
                        QS_OBJ_PRE_(act);      // the target AO
                        QS_U8_PRE_(tickRate);  // tick rate
                    QS_END_PRE_()
                }

                QS_BEGIN_PRE_(QS_QF_TIMEEVT_POST, act->m_prio)
                    QS_TIME_PRE_();            // timestamp
                    QS_OBJ_PRE_(e);            // the time event object
                    QS_SIG_PRE_(e->sig);       // signal of this time event
                    QS_OBJ_PRE_(act);          // the target AO
                    QS_U8_PRE_(tickRate);      // tick rate
                QS_END_PRE_()

    #ifdef QXK_HPP_
                if (e->sig < Q_USER_SIG) {
                    QXThread::timeout_(act);
                    QF_MEM_APP();
                    QF_CRIT_EXIT();
                }
                else {
                    QF_MEM_APP();
                    QF_CRIT_EXIT(); // exit crit. section before posting

                    // act->POST() asserts if the queue overflows
                    act->POST(e, sender);
                }
    #else
                QF_MEM_APP();
                QF_CRIT_EXIT(); // exit crit. section before posting

                // act->POST() asserts if the queue overflows
                act->POST(e, sender);
    #endif
            }
            else {
                prev = e; // advance to this time event

                QF_MEM_APP();
                QF_CRIT_EXIT(); // exit crit. section to reduce latency

                // prevent merging critical sections, see NOTE above
                QF_CRIT_EXIT_NOP();
            }
        }
        QF_CRIT_ENTRY(); // re-enter crit. section to continue the loop
        QF_MEM_SYS();
    }

    Q_ENSURE_INCRIT(190, lbound > 0U);
    QF_MEM_APP();
    QF_CRIT_EXIT();
}

//${QF::QTimeEvt::noActive} ..................................................
bool QTimeEvt::noActive(std::uint_fast8_t const tickRate) noexcept {
    QF_CRIT_STAT
    QF_CRIT_ENTRY();
    Q_REQUIRE_INCRIT(800, tickRate < QF_MAX_TICK_RATE);
    QF_CRIT_EXIT();

    bool inactive;
    if (timeEvtHead_[tickRate].m_next != nullptr) {
        inactive = false;
    }
    else if (timeEvtHead_[tickRate].m_act != nullptr) {
        inactive = false;
    }
    else {
        inactive = true;
    }
    return inactive;
}

//${QF::QTimeEvt::QTimeEvt} ..................................................
QTimeEvt::QTimeEvt() noexcept
 :
    QEvt(0U),
    m_next(nullptr),
    m_act(nullptr),
    m_ctr(0U),
    m_interval(0U)
{
    // The refCtr_ attribute is not used in time events, so it is
    // reused to hold the tickRate as well as other information
    refCtr_ = 0U; // default rate 0
}

} // namespace QP
//$enddef${QF::QTimeEvt} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
