#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity): 
    cur(0), remain(0), buff(capacity, 0), bitmap(capacity, false), isEof(false), _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if(eof) isEof = true;
    if(index >= cur && index < cur + _capacity) {
        size_t offset = index - cur;
        size_t min = std::min(data.size(), _capacity - _output.buffer_size() - offset);
        if(min < data.size()) isEof = false;
        for(size_t i = 0; i < min; ++i) {
            if(!bitmap[i + offset]) {
                buff[i + offset] = data[i];
                bitmap[i + offset] = true;
                ++remain;
            }
        }
    } else if(index < cur && index + data.size() > cur) {
        size_t offset = cur - index;
        size_t min = std::min(data.size() - offset, _capacity - _output.buffer_size());
        if(min < data.size() - offset) isEof = false;
        for(size_t i = 0; i < min; ++i) {
            if(!bitmap[i]) {
                buff[i] = data[i + offset];
                bitmap[i] = true;
                ++remain;
            }
        }
    }
    string s;
    s.clear();
    while(bitmap.front()) {
        s.push_back(buff.front());
        ++cur, --remain;
        buff.pop_front(), buff.push_back('\0');
        bitmap.pop_front(), bitmap.push_back(false);
    }
    if(s.size() > 0) _output.write(s);
    if(isEof && remain == 0) _output.end_input(); 
}

size_t StreamReassembler::unassembled_bytes() const { return remain; }

bool StreamReassembler::empty() const { return remain == 0; }
