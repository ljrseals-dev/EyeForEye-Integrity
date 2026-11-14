#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <vector>

namespace eye {

// Thread-safe bounded map. Operations are serialized by an internal mutex;
// record() is NOT real-time safe and must not be called from an audio callback.
class PositionMap {
public:
    static constexpr std::size_t kCapacity = 8192;
    static constexpr std::uint32_t kFormatVersion = 1;
    static constexpr std::uint32_t kMagic = 0x504D4150u;

    struct Entry { double beat = 0.0; std::uint32_t syllable = 0; };

    PositionMap() noexcept = default;
    void record(double beat, std::uint32_t syllable) noexcept;
    std::optional<std::uint32_t> lookup(double beat) const noexcept;
    void freeze() noexcept;
    void unfreeze() noexcept;
    bool isFrozen() const noexcept;
    void reset() noexcept;
    std::size_t size() const noexcept;
    bool empty() const noexcept;
    std::vector<std::byte> serialize() const;
    bool deserialize(const std::vector<std::byte>& data) noexcept;

private:
    mutable std::mutex mutex_;
    std::array<Entry, kCapacity> entries_{};
    std::size_t count_ = 0;
    bool frozen_ = false;
};

} // namespace eye
