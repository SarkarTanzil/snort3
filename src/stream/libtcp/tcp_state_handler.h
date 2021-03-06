//--------------------------------------------------------------------------
// Copyright (C) 2015-2016 Cisco and/or its affiliates. All rights reserved.
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License Version 2 as published
// by the Free Software Foundation.  You may not use, modify or distribute
// this program under any other version of the GNU General Public License.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//--------------------------------------------------------------------------

// tcp_state_handler.h author davis mcpherson <davmcphe@@cisco.com>
// Created on: Jun 24, 2015

#ifndef TCP_STATE_HANDLER_H
#define TCP_STATE_HANDLER_H

#include "protocols/tcp.h"
#include "tcp_segment_descriptor.h"
#include "tcp_stream_session.h"
#include "tcp_stream_tracker.h"

class TcpStateMachine;

class TcpStateHandler
{
public:
    TcpStateHandler(TcpStreamTracker::TcpState, TcpStateMachine&, TcpStreamSession&);
    virtual ~TcpStateHandler();

    virtual bool eval(TcpSegmentDescriptor&, TcpStreamTracker&);

    TcpStreamTracker::TcpEvent get_tcp_event() const
    {
        return tcp_event;
    }

    TcpStreamTracker::TcpState get_tcp_state() const
    {
        return tcp_state;
    }

    void set_tcp_event(TcpStreamTracker::TcpEvent tcp_event)
    {
        this->tcp_event = tcp_event;
    }

    const TcpStateMachine* get_tsm() const
    {
        return tsm;
    }

    void set_tsm(const TcpStateMachine* tsm)
    {
        this->tsm = tsm;
    }

    virtual bool do_pre_sm_packet_actions(TcpSegmentDescriptor&);
    virtual bool do_post_sm_packet_actions(TcpSegmentDescriptor&);

protected:
    virtual bool syn_sent(TcpSegmentDescriptor&, TcpStreamTracker&);
    virtual bool syn_recv(TcpSegmentDescriptor&, TcpStreamTracker&);
    virtual bool syn_ack_sent(TcpSegmentDescriptor&, TcpStreamTracker&);
    virtual bool syn_ack_recv(TcpSegmentDescriptor&, TcpStreamTracker&);
    virtual bool ack_sent(TcpSegmentDescriptor&, TcpStreamTracker&);
    virtual bool ack_recv(TcpSegmentDescriptor&, TcpStreamTracker&);
    virtual bool data_seg_sent(TcpSegmentDescriptor&, TcpStreamTracker&);
    virtual bool data_seg_recv(TcpSegmentDescriptor&, TcpStreamTracker&);
    virtual bool fin_sent(TcpSegmentDescriptor&, TcpStreamTracker&);
    virtual bool fin_recv(TcpSegmentDescriptor&, TcpStreamTracker&);
    virtual bool rst_sent(TcpSegmentDescriptor&, TcpStreamTracker&);
    virtual bool rst_recv(TcpSegmentDescriptor&, TcpStreamTracker&);

    bool default_state_action(TcpSegmentDescriptor&, TcpStreamTracker&);

    const TcpStateMachine* tsm;
    TcpStreamTracker::TcpState tcp_state;
    TcpStreamSession& session;
    TcpStreamTracker::TcpEvent tcp_event = TcpStreamTracker::TCP_MAX_EVENTS;
};

#endif

