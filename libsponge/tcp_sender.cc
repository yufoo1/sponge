#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const { return _outgoing_bytes; }

void TCPSender::fill_window() {
    size_t cur_window_size = _last_window_size == 0 ? 1 : _last_window_size;
    while(cur_window_size > _outgoing_bytes) {
        TCPSegment seg;
        if(!_has_syn) _has_syn = seg.header().syn = true;
        seg.header().seqno = next_seqno();
        std::string payload = _stream.read(min(TCPConfig::MAX_PAYLOAD_SIZE, cur_window_size - _outgoing_bytes - seg.header().syn));
        if(!_has_fin && _stream.eof() && payload.size() + _outgoing_bytes < cur_window_size) _has_fin = seg.header().fin = true;
        seg.payload() = Buffer(std::move(payload));
        if(seg.length_in_sequence_space() == 0) break;
        if(_outgoing_map.empty()) {
            _timeout =  _initial_retransmission_timeout;
            _timecount = 0;
        }
        _segments_out.push(seg);
        _outgoing_bytes += seg.length_in_sequence_space();
        _outgoing_map.insert(make_pair(_next_seqno, seg));
        _next_seqno += seg.length_in_sequence_space();
        if(seg.header().fin) break;
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 
    uint64_t abs_seqno = unwrap(ackno, _isn, _next_seqno);
    if(abs_seqno > _next_seqno) return;
    auto i = _outgoing_map.begin();
    while(i != _outgoing_map.end()) {
        const TCPSegment &seg = i->second;
        if(abs_seqno >= i->first + seg.length_in_sequence_space()) {
            i = _outgoing_map.erase(i);
            _outgoing_bytes -= seg.length_in_sequence_space();
            _timeout = _initial_retransmission_timeout;
            _timecount = 0;
        } else break;
    }
    _consecutive_retransmissions = 0;
    _last_window_size = window_size;
    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { 
    _timecount += ms_since_last_tick;
    auto i = _outgoing_map.begin();
    if(i != _outgoing_map.end() && _timecount >= _timeout) {
        if(_last_window_size > 0) _timeout *= 2;
        _timecount = 0;
        _segments_out.push(i->second);
        ++_consecutive_retransmissions;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = next_seqno();
    _segments_out.push(seg);
}
