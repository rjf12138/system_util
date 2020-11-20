#ifndef __BUFFER_H__
#define __BUFFER_H__

#include "basic_head.h"

namespace my_utils {

#define MAX_BUFFER_SIZE     1073741824 // 1*1024*1024*1024 (1GB)
#define MAX_DATA_SIZE       1073741823 // 多的一个字节用于防止，缓存写满时，start_write 和 start_read 重合而造成分不清楚是写满了还是没写

typedef int64_t     BUFSIZE_T;
typedef uint64_t    UNBUFSIZE_T;
typedef int8_t      BUFFER_TYPE;
typedef int8_t*     BUFFER_PTR;

class ByteBuffer_Iterator;
class ByteBuffer {
    friend class ByteBuffer_Iterator;
public:
    ByteBuffer(BUFSIZE_T size = 0);
    ByteBuffer(const ByteBuffer &buff);
    virtual ~ByteBuffer();

    BUFSIZE_T read_only_int8(int8_t &val) {return 0;}
    BUFSIZE_T read_only_int16(int16_t &val) {return 0;}
    BUFSIZE_T read_only_int32(int32_t &val) {return 0;}
    BUFSIZE_T read_only_int64(int64_t &val) {return 0;}
    BUFSIZE_T read_only_string(string &str) {return 0;}
    BUFSIZE_T read_only_bytes(void *buf, BUFSIZE_T buf_size, bool match = false) {return 0;}

    BUFSIZE_T read_int8(int8_t &val);
    BUFSIZE_T read_int16(int16_t &val);
    BUFSIZE_T read_int32(int32_t &val);
    BUFSIZE_T read_int64(int64_t &val);
    BUFSIZE_T read_string(string &str, BUFSIZE_T str_size = -1);
    BUFSIZE_T read_bytes(void *buf, BUFSIZE_T buf_size, bool match = false);

    BUFSIZE_T write_int8(int8_t val);
    BUFSIZE_T write_int16(int16_t val);
    BUFSIZE_T write_int32(int32_t val);
    BUFSIZE_T write_int64(int64_t val);
    BUFSIZE_T write_string(const string &str, BUFSIZE_T str_size = -1);
    BUFSIZE_T write_bytes(const void *buf, BUFSIZE_T buf_size, bool match = false);

    // 网络字节序转换
    // 将缓存中的数据读取出来并转成主机字节序返回
    int read_int16_ntoh(int16_t &val);
    int read_int32_ntoh(int32_t &val);
    // 将主机字节序转成网络字节序写入缓存
    int write_int16_hton(const int16_t &val);
    int write_int32_hton(const int32_t &val);

    bool empty(void) const;
    BUFSIZE_T data_size(void) const;
    BUFSIZE_T idle_size(void) const;
    BUFSIZE_T clear(void);
    BUFSIZE_T set_extern_buffer(BUFFER_PTR exbuf, int buff_size);

    // 重新分配缓冲区大小(只能向上增长), size表示重新分配缓冲区的下限
    BUFSIZE_T resize(BUFSIZE_T size);
    
    // 返回起始结束迭代器
    ByteBuffer_Iterator begin(void) const;
    ByteBuffer_Iterator end(void) const;

    // 重载操作符
    friend ByteBuffer operator+(ByteBuffer &lhs, ByteBuffer &rhs);
    friend bool operator==(const ByteBuffer &lhs, const ByteBuffer &rhs);
    friend bool operator!=(const ByteBuffer &lhs, const ByteBuffer &rhs);
    ByteBuffer& operator=(const ByteBuffer& src);

    // 向外面直接提供 buffer_ 指针，它们写是直接写入指针，避免不必要的拷贝
    BUFFER_PTR get_write_buffer_ptr(void) const;
    BUFFER_PTR get_read_buffer_ptr(void) const;

    // ByteBuffer 是循环队列，读写不一定是连续的
    BUFSIZE_T get_cont_write_size(void) const;
    BUFSIZE_T get_cont_read_size(void) const;

    // 更新读写数据和剩余的缓冲大小
    void update_write_pos(BUFSIZE_T offset);
    void update_read_pos(BUFSIZE_T offset);

private:
    // 下一个读的位置
    void next_read_pos(int offset = 1);
    // 下一个写的位置
    void next_write_pos(int offset = 1);

    // 将data中的数据拷贝size个字节到当前bytebuff中
    BUFSIZE_T copy_data_to_buffer(const void *data, BUFSIZE_T size);
    // 从bytebuff中拷贝data个字节到data中
    BUFSIZE_T copy_data_from_buffer(void *data, BUFSIZE_T size);

private:
    BUFFER_PTR buffer_;

    BUFSIZE_T start_read_pos_;
    BUFSIZE_T start_write_pos_;

    BUFSIZE_T used_data_size_;
    BUFSIZE_T free_data_size_;
    BUFSIZE_T max_buffer_size_;

    shared_ptr<ByteBuffer_Iterator> bytebuff_iter_start_;
    shared_ptr<ByteBuffer_Iterator> bytebuff_iter_end_;
};

////////////////////////// ByteBuffer 迭代器 //////////////////////////////////////
class ByteBuffer_Iterator : public iterator<random_access_iterator_tag, int8_t> 
{
    friend class ByteBuffer;
public:
    ByteBuffer_Iterator(void)
        : buff_(nullptr), curr_pos_(0) {}
    explicit ByteBuffer_Iterator(const ByteBuffer *buff)
            : buff_(buff), curr_pos_(buff->start_read_pos_){}

    ByteBuffer_Iterator begin() 
    {
        ByteBuffer_Iterator tmp = *this;
        tmp.curr_pos_ = buff_->start_read_pos_;
        return tmp;
    }

    ByteBuffer_Iterator end()
    {
        ByteBuffer_Iterator tmp = *this;
        tmp.curr_pos_ = buff_->start_write_pos_;
        return tmp;
    }

    int8_t operator*()
    {
        if (this->check_iterator() == false) {
            ostringstream ostr;
            ostr << "Line: " << __LINE__ << " ByteBuffer_Iterator operator+ out of range.";
            ostr << "debug_info: " << this->debug_info() << std::endl;
            throw runtime_error(ostr.str());
        }
        return buff_->buffer_[curr_pos_];
    }

    ByteBuffer_Iterator operator+(BUFSIZE_T inc)
    {
        if (this->check_iterator() == false) {
            return *this;
        }

        ByteBuffer_Iterator tmp_iter = *this;
        tmp_iter.curr_pos_ = (tmp_iter.curr_pos_ + buff_->max_buffer_size_ + inc)  % buff_->max_buffer_size_;
        tmp_iter.check_iterator();

        return tmp_iter;
    }

    ByteBuffer_Iterator operator-(int des) {
        if (this->check_iterator() == false) {
            return *this;
        }

        ByteBuffer_Iterator tmp_iter = *this;
        tmp_iter.curr_pos_ = (tmp_iter.curr_pos_ + buff_->max_buffer_size_ - des)  % buff_->max_buffer_size_;
        tmp_iter.check_iterator();


        return tmp_iter;
    }

    // 前置++
    ByteBuffer_Iterator& operator++()
    {
        if (this->check_iterator() == false)
        {
            return *this;
        }

        curr_pos_ = (curr_pos_ + buff_->max_buffer_size_ + 1) % buff_->max_buffer_size_;
        this->check_iterator();

        return *this;
    }

    // 后置++
    ByteBuffer_Iterator operator++(int)
    {
        if (this->check_iterator() == false)
        {
            return *this;
        }

        ByteBuffer_Iterator tmp = *this;
        curr_pos_ = (curr_pos_ + buff_->max_buffer_size_ + 1) % buff_->max_buffer_size_;
        this->check_iterator();

        return tmp;
    }

    // 前置--
    ByteBuffer_Iterator& operator--()
    {
        if (this->check_iterator() == false) {
            return *this;
        }

        curr_pos_ = (curr_pos_ + buff_->max_buffer_size_ - 1) % buff_->max_buffer_size_;
        this->check_iterator();

        return *this;
    }

    // 后置--
    ByteBuffer_Iterator operator--(int)
    {
        if (this->check_iterator() == false) {
            return *this;
        }

        ByteBuffer_Iterator tmp = *this;
        curr_pos_ = (curr_pos_ + buff_->max_buffer_size_ - 1) % buff_->max_buffer_size_;
        this->check_iterator();

        return tmp;
    }

    // +=
    ByteBuffer_Iterator& operator+=(BUFSIZE_T inc)
    {
        if (this->check_iterator() == false) {
            return *this;
        }

        curr_pos_ = (curr_pos_ + buff_->max_buffer_size_ + inc)  % buff_->max_buffer_size_;
        this->check_iterator();

        return *this;
    }

    ByteBuffer_Iterator& operator-=(BUFSIZE_T des)
    {
        if (this->check_iterator() == false) {
            return *this;
        }

        curr_pos_ = (curr_pos_ + buff_->max_buffer_size_ - des)  % buff_->max_buffer_size_;
        this->check_iterator();

        return *this;
    }

    // 只支持 == ,!= , = 其他的比较都不支持
    bool operator==(const ByteBuffer_Iterator& iter) const {
        return (curr_pos_ == iter.curr_pos_ && buff_ == iter.buff_);
    }
    bool operator!=(const ByteBuffer_Iterator& iter) const {
        return (curr_pos_ != iter.curr_pos_ || buff_ != iter.buff_);
    }
    bool operator>(const ByteBuffer_Iterator& iter) const {
        if (buff_ != iter.buff_) {
            return false;
        }
        if (curr_pos_ > iter.curr_pos_) {
            return true;
        } else if (curr_pos_ < iter.curr_pos_) {
            if (curr_pos_ < buff_->start_read_pos_) {
                return true;
            }
        }

        return false;
    }
    bool operator>=(const ByteBuffer_Iterator& iter) const {
        if (buff_ != iter.buff_) {
            return false;
        }
        if (curr_pos_ >= iter.curr_pos_) {
            return true;
        } else if (curr_pos_ < iter.curr_pos_) {
            if (curr_pos_ < buff_->start_read_pos_) {
                return true;
            }
        }

        return false;
    }
    bool operator<(const ByteBuffer_Iterator& iter) const {
        if (buff_ != iter.buff_) {
            return false;
        }
        if (curr_pos_ >= iter.curr_pos_) {
            return false;
        } else if (curr_pos_ < iter.curr_pos_) {
            if (curr_pos_ < buff_->start_read_pos_) {
                return false;
            }
        }

        return true;
    }
    bool operator<=(const ByteBuffer_Iterator& iter) const {
        if (buff_ != iter.buff_) {
            return false;
        }
        if (curr_pos_ > iter.curr_pos_) {
            return false;
        } else if (curr_pos_ < iter.curr_pos_) {
            if (curr_pos_ < buff_->start_read_pos_) {
                return false;
            }
        }

        return true;
    }
    ByteBuffer_Iterator& operator=(const ByteBuffer_Iterator& src)
    {
        if (src != *this) {
            buff_ = src.buff_;
            curr_pos_ = src.curr_pos_;
        }

        return *this;
    }
    
    string debug_info(void) {
        ostringstream ostr;

        ostr << std::endl << "--------------debug_info-----------------------" << std::endl;
        ostr << "curr_pos: " << curr_pos_ << std::endl;
        ostr << "begin_pos: " << buff_->begin().curr_pos_ << std::endl;
        ostr << "end_pos: " << buff_->end().curr_pos_ << std::endl;
        ostr << "buff_length: "  << buff_->data_size() << std::endl;
        ostr << "------------------------------------------------" << std::endl;

        return ostr.str();
    }
private:
    bool check_iterator(void) {
        if (buff_ == nullptr || buff_->buffer_ == nullptr) {
            curr_pos_ = 0;
            return false;
        }

        if (buff_->start_write_pos_ >= buff_->start_read_pos_) {
            if (curr_pos_ < buff_->start_read_pos_ || curr_pos_ >= buff_->start_write_pos_) {
                curr_pos_ = buff_->start_write_pos_;
                return false;
            }
        }

        if (buff_->start_write_pos_ < buff_->start_read_pos_) {
            if (curr_pos_ < buff_->start_read_pos_ && curr_pos_ >= buff_->start_write_pos_) {
                curr_pos_ = buff_->start_write_pos_;
                return false;
            }
        }

        return true;
    }
private:
    const ByteBuffer *buff_ = nullptr;
    BUFSIZE_T curr_pos_;
};

}

#endif