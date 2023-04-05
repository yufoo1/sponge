#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _time_since_last_segment_received; }

void TCPConnection::segment_received(const TCPSegment &seg) { 
    _time_since_last_segment_received = 0;
    bool need_send_ack = seg.length_in_sequence_space() != 0;
    _receiver.segment_received(seg);
    if(seg.header().rst) {
        _set_rst(false);
        return;
    }

    if(seg.header().ack) {
        _sender.ack_received(seg.header().ackno, seg.header().win);
        if(need_send_ack && !_sender.segments_out().empty()) need_send_ack = false;
    }

    if(TCPState::state_summary(_receiver) == TCPReceiverStateSummary::SYN_RECV &&
        TCPState::state_summary(_sender) == TCPSenderStateSummary::CLOSED) {
        connect();
        return;
    }

    if (TCPState::state_summary(_receiver) == TCPReceiverStateSummary::FIN_RECV &&
        TCPState::state_summary(_sender) == TCPSenderStateSummary::SYN_ACKED) {
        _linger_after_streams_finish = false;
    }

    if (TCPState::state_summary(_receiver) == TCPReceiverStateSummary::FIN_RECV &&
        TCPState::state_summary(_sender) == TCPSenderStateSummary::FIN_ACKED && 
        !_linger_after_streams_finish) {
        _is_active = false;
        return;
    }

    if (need_send_ack) {
        _sender.send_empty_segment();
    }

    _send_ack_and_window_size();
}

bool TCPConnection::active() const { return _is_active; }

size_t TCPConnection::write(const string &data) {
    size_t size = _sender.stream_in().write(data);
    _sender.fill_window();
    _send_ack_and_window_size();
    return size;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) { 
    _sender.tick(ms_since_last_tick);
    if(_sender.consecutive_retransmissions() > _cfg.MAX_RETX_ATTEMPTS) {
        _sender.segments_out().pop();
        _set_rst(true);
        return;
    }

    _send_ack_and_window_size();
    _time_since_last_segment_received += ms_since_last_tick;

    if(TCPState::state_summary(_receiver) == TCPReceiverStateSummary::FIN_RECV && 
        TCPState::state_summary(_sender) == TCPSenderStateSummary::FIN_ACKED && 
        _linger_after_streams_finish && _time_since_last_segment_received >= 10 * _cfg.rt_timeout) {
            _is_active = false;
            _linger_after_streams_finish = false;
    }
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
    _send_ack_and_window_size();
}

void TCPConnection::connect() {
    _is_active = true;
    _sender.fill_window();
    _send_ack_and_window_size();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
            _set_rst(false);
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

void TCPConnection::_set_rst(bool rst) {
    if(rst) {
        TCPSegment seg;
        seg.header().rst = true;
        _segments_out.push(seg);
    }
    _receiver.stream_out().set_error();
    _sender.stream_in().set_error();
    _linger_after_streams_finish = false;
    _is_active = false;
}

void TCPConnection::_send_ack_and_window_size() {
    while(!_sender.segments_out().empty()) {
        TCPSegment seg = _sender.segments_out().front();
        _sender.segments_out().pop();
        if(_receiver.ackno().has_value()) {
            seg.header().ack = true;
            seg.header().ackno = _receiver.ackno().value();
            seg.header().win = _receiver.window_size();
        }
        _segments_out.push(seg);
    }
}
