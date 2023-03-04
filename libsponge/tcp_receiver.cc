#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    const TCPHeader &header = seg.header();
    if(!hasSyn) {
        if(!header.syn) return;
        _isn = header.seqno;
        hasSyn = true;
    }
    uint64_t checkpoint = _reassembler.stream_out().bytes_written() + 1;
    uint64_t curAbsSeqNo = unwrap(header.seqno, _isn, checkpoint);
    uint64_t absSeqNo = curAbsSeqNo - 1 + uint64_t(header.syn);
    _reassembler.push_substring(seg.payload().copy(), absSeqNo, header.fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if(!hasSyn) return std::nullopt;
    uint64_t ackNo = _reassembler.stream_out().bytes_written() + 1;
    if (_reassembler.stream_out().input_ended()) ++ackNo;
    return WrappingInt32(_isn) + ackNo;
}

size_t TCPReceiver::window_size() const { return _capacity - _reassembler.stream_out().buffer_size(); }
