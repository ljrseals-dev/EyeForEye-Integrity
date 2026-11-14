#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <vector>
namespace eye {
// Bounded and mutex-protected. This is deliberately NOT real-time safe:
// record/reset/deserialize may block and must never run in an audio callback.
class PositionMap {
public:
 static constexpr std::size_t kCapacity=8192;
 static constexpr std::uint32_t kFormatVersion=1,kMagic=0x504D4150u;
 struct Entry{double beat=0.0;std::uint32_t syllable=0;};
 PositionMap() noexcept=default;
 void record(double,std::uint32_t);
 std::optional<std::uint32_t> lookup(double)const;
 void freeze();void unfreeze();bool isFrozen()const;void reset();std::size_t size()const;bool empty()const;
 std::vector<std::byte> serialize()const;
 bool deserialize(const std::vector<std::byte>&);
private:
 mutable std::mutex mutex_;std::array<Entry,kCapacity>entries_{};std::size_t count_=0;bool frozen_=false;
};
} // namespace eye
