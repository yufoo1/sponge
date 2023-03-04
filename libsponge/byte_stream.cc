#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity): 
    buff(), capa(capacity), wrBytes(0), rdBytes(0), wrEnd(false), rdEnd(false) {}

size_t ByteStream::write(const string &data) {
    size_t remain = capa - buff.size();
    size_t min = std::min(remain, data.size());
    for(size_t i = 0; i < min; ++i) buff.push_back(data[i]);
    wrBytes += min;
    return min;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
	std::string s;
	for(size_t i = 0; i < std::min(len, buff.size()); ++i) s += buff[i];
	return s;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
	size_t min = std::min(len, buff.size());
	for(size_t i = 0; i < min; ++i) buff.pop_front();
	rdBytes += min;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
	string s = this->peek_output(len);
    pop_output(len);
    return s;
}

void ByteStream::end_input() { wrEnd = true; }

bool ByteStream::input_ended() const { return wrEnd; }

size_t ByteStream::buffer_size() const { return buff.size(); }

bool ByteStream::buffer_empty() const { return buff.empty(); }

bool ByteStream::eof() const { return buff.empty() && wrEnd; }

size_t ByteStream::bytes_written() const { return wrBytes; }

size_t ByteStream::bytes_read() const { return rdBytes; }

size_t ByteStream::remaining_capacity() const { return capa - buff.size(); }
