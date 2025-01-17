#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "sung/basic/optional.hpp"


namespace sung {

    bool is_big_endian() noexcept;
    bool is_little_endian() noexcept;


    template <typename T>
    T flip_byte_order(T value) {
        T result = 0;
        for (size_t i = 0; i < sizeof(T); ++i) {
            result <<= 8;
            result |= value & 0xFF;
            value >>= 8;
        }
        return result;
    }

    template <>
    inline float flip_byte_order(float value) {
        uint32_t result = flip_byte_order(*reinterpret_cast<uint32_t*>(&value));
        return *reinterpret_cast<float*>(&result);
    }

    template <>
    inline double flip_byte_order(double value) {
        uint64_t result = flip_byte_order(*reinterpret_cast<uint64_t*>(&value));
        return *reinterpret_cast<double*>(&result);
    }


    template <typename T>
    T assemble_be_data(const uint8_t* buf) {
        if (is_big_endian()) {
            return *reinterpret_cast<const T*>(buf);
        } else {
            uint8_t tmp[sizeof(T)];
            for (size_t i = 0; i < sizeof(T); ++i)
                tmp[i] = buf[sizeof(T) - i - 1];
            return *reinterpret_cast<const T*>(tmp);
        }
    }

    template <typename T>
    T assemble_le_data(const uint8_t* buf) {
        if (is_little_endian()) {
            return *reinterpret_cast<const T*>(buf);
        } else {
            uint8_t tmp[sizeof(T)];
            for (size_t i = 0; i < sizeof(T); ++i)
                tmp[i] = buf[sizeof(T) - i - 1];
            return *reinterpret_cast<const T*>(tmp);
        }
    }

    template <typename T>
    bool decompose_to_be(T src, uint8_t* dst, size_t dst_size) {
        if (dst_size < sizeof(T))
            return false;

        if (is_big_endian()) {
            std::memcpy(dst, &src, sizeof(T));
        } else {
            auto src_ptr = reinterpret_cast<const uint8_t*>(&src);
            for (size_t i = 0; i < sizeof(T); ++i)
                dst[i] = src_ptr[sizeof(T) - i - 1];
        }

        return true;
    }

    template <typename T>
    bool decompose_to_le(T src, uint8_t* dst, size_t dst_size) {
        if (dst_size < sizeof(T))
            return false;

        if (is_little_endian()) {
            std::memcpy(dst, &src, sizeof(T));
        } else {
            auto src_ptr = reinterpret_cast<const uint8_t*>(&src);
            for (size_t i = 0; i < sizeof(T); ++i)
                dst[i] = src_ptr[sizeof(T) - i - 1];
        }

        return true;
    }


    // Little endian
    class BytesBuilder {

    public:
        size_t size() const noexcept;
        uint8_t* data() noexcept;
        const uint8_t* data() const noexcept;
        const std::vector<uint8_t>& vector() const noexcept;

        void clear() noexcept { data_.clear(); }

        // Once you call this, it is safe to call this->clear() before calling
        // any other methods
        std::vector<uint8_t>&& release() noexcept { return std::move(data_); }

        void enlarge(size_t size);

        std::pair<size_t, size_t> add_arr(const void* src, size_t size);

        template <typename T>
        std::pair<size_t, size_t> add_arr(const T& src) {
            return this->add_arr(src.data(), src.size());
        }

        template <typename T>
        std::pair<size_t, size_t> add_val_arr(const T* src, size_t size) {
            if (is_little_endian()) {
                return this->add_arr(src, sizeof(T) * size);
            } else {
                const auto start_pos = data_.size();
                for (size_t i = 0; i < size; ++i) {
                    this->add_val(src[i]);
                }
                return { start_pos, sizeof(T) * size };
            }
        }

        template <typename T>
        std::pair<size_t, size_t> add_val_arr(const T& src) {
            return this->add_val_arr(src.data(), src.size());
        }

        // Add a null-terminated string
        void add_nt_str(const char* const str);

        template <typename T>
        void add_val(T val) {
            this->enlarge(sizeof(T));
            decompose_to_le(
                val, data_.data() + data_.size() - sizeof(T), sizeof(T)
            );
        }

        void add_int8(int8_t val) { this->add_val(val); }
        void add_int16(int16_t val) { this->add_val(val); }
        void add_int32(int32_t val) { this->add_val(val); }
        void add_int64(int64_t val) { this->add_val(val); }

        void add_uint8(uint8_t val) { this->add_val(val); }
        void add_uint16(uint16_t val) { this->add_val(val); }
        void add_uint32(uint32_t val) { this->add_val(val); }
        void add_uint64(uint64_t val) { this->add_val(val); }

        void add_float32(float val) { this->add_val(val); }
        void add_float64(double val) { this->add_val(val); }

    private:
        std::vector<uint8_t> data_;
    };


    // Little endian
    class BytesReader {

    public:
        BytesReader(const uint8_t* data, size_t size);

        size_t size() const noexcept { return size_; }
        size_t remaining() const noexcept { return size_ - pos_; }
        const uint8_t* data() const noexcept { return data_; }
        const uint8_t* head() const noexcept { return data_ + pos_; }

        bool is_eof() const noexcept;
        bool has_overflow() const noexcept;

        void advance(size_t size) { pos_ += size; }

        std::string read_nt_str();

        template <typename T>
        sung::Optional<T> read_val() {
            if (pos_ + sizeof(T) > size_)
                return sung::nullopt;

            const auto out = assemble_le_data<T>(data_ + pos_);
            pos_ += sizeof(T);
            return out;
        }

        template <typename T>
        bool read_val_arr(T* dst, size_t count) {
            const auto read_size = sizeof(T) * count;
            if (pos_ + read_size > size_)
                return false;

            if (is_little_endian()) {
                std::memcpy(dst, data_ + pos_, read_size);
                pos_ += read_size;
            } else {
                for (size_t i = 0; i < count; ++i) {
                    dst[i] = assemble_le_data<T>(data_ + pos_);
                    pos_ += sizeof(T);
                }
            }

            return true;
        }

        template <typename T, size_t TSize>
        sung::Optional<std::array<T, TSize>> read_val_arr() {
            std::array<T, TSize> out;
            if (this->read_val_arr<T>(out.data(), TSize))
                return out;
            return sung::nullopt;
        }

        bool read_raw_arr(uint8_t* dst, size_t count) {
            if (pos_ + count > size_)
                return false;

            std::memcpy(dst, data_ + pos_, count);
            pos_ += count;
            return true;
        }

        sung::Optional<int8_t> read_bool() { return read_val<int8_t>() != 0; }

        sung::Optional<int32_t> read_int32() { return read_val<int32_t>(); }
        sung::Optional<int64_t> read_int64() { return read_val<int64_t>(); }
        sung::Optional<uint32_t> read_uint32() { return read_val<uint32_t>(); }
        sung::Optional<uint64_t> read_uint64() { return read_val<uint64_t>(); }

        sung::Optional<float> read_float32() { return read_val<float>(); }
        sung::Optional<double> read_float64() { return read_val<double>(); }

        bool read_int32_arr(int32_t* dst, size_t count) {
            return read_val_arr<int32_t>(dst, count);
        }
        bool read_float32_arr(float* dst, size_t count) {
            return read_val_arr<float>(dst, count);
        }

        template <size_t TSize>
        sung::Optional<std::array<float, TSize>> read_float32_arr() {
            std::array<float, TSize> out;
            if (this->read_val_arr(out.data(), TSize))
                return out;
            return sung::nullopt;
        }

    private:
        const uint8_t* const data_;
        const size_t size_;
        size_t pos_ = 0;
    };


    template <typename T>
    class BEValue {

    public:
        BEValue() = default;

        BEValue(const T& value) {
            decompose_to_be(value, data_.data(), data_.size());
        }

        T get() const { return assemble_be_data<T>(data_.data()); }

        void set(T value) {
            decompose_to_be(value, data_.data(), data_.size());
        }

        const uint8_t* data() const { return data_.data(); }
        size_t size() const { return data_.size(); }

    private:
        std::array<uint8_t, sizeof(T)> data_ = { 0 };
    };

}  // namespace sung
