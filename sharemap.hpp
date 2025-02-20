#pragma once

#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <array>
#include <chrono>
#include <string>
#include <string_view>
#include <type_traits>

namespace anysignal {

static constexpr std::size_t STRING_BUFFER_SIZE = 64;

template <typename T, std::enable_if_t<std::is_same_v<T, std::array<char, STRING_BUFFER_SIZE>>, bool> = true>
void sharemap_pack_field(const T &in, std::uint8_t *out)
{
    std::memset(out, 0, STRING_BUFFER_SIZE);

    // Make sure string is nul-terminated.
    std::memcpy((char *)out, in.data(), in.max_size() - 1);
}

template <typename T, std::enable_if_t<std::is_same_v<T, bool>, bool> = true>
void sharemap_pack_field(const T &in, std::uint8_t *out)
{
    *out = in?1:0;
}

template <typename T, std::enable_if_t<std::is_integral_v<T> and not std::is_same_v<T, bool>, bool> = true>
void sharemap_pack_field(const T &in, std::uint8_t *out)
{
    for (std::size_t i = 0; i < sizeof(T); ++i)
    {
        out[i] = std::uint8_t(in >> ((sizeof(T) - i - 1) * CHAR_BIT) & 0xFF);
    }
}

template <typename T, std::enable_if_t<std::is_floating_point_v<T>, bool> = true>
void sharemap_pack_field(const T &in, std::uint8_t *out)
{
    std::memcpy(out, &in, sizeof(T));
}

template <typename T, std::enable_if_t<std::is_same_v<T, std::array<char, STRING_BUFFER_SIZE>>, bool> = true>
void sharemap_unpack_field(const std::uint8_t *in, T &out)
{
    // We use nul-terminated byte strings.
    std::memcpy(reinterpret_cast<char *>(out.data()), in, out.max_size() - 1);
    out[out.max_size() - 1] = '\0';
}

template <typename T, std::enable_if_t<std::is_same_v<T, bool>, bool> = true>
void sharemap_unpack_field(const std::uint8_t *in, T &out)
{
    out = (*in) != 0;
}

template <typename T, std::enable_if_t<std::is_integral_v<T> and not std::is_same_v<T, bool>, bool> = true>
void sharemap_unpack_field(const std::uint8_t *in, T &out)
{
    out = T{};
    for (std::size_t i = 0; i < sizeof(T); ++i)
    {
        out |= T{in[i]} << ((sizeof(T) - i - 1) * CHAR_BIT);
    }
}

template <typename T, std::enable_if_t<std::is_floating_point_v<T>, bool> = true>
void sharemap_unpack_field(const std::uint8_t *in, T &out)
{
    std::memcpy(&out, in, sizeof(T));
}

[[nodiscard]] static inline std::int64_t time_ns_since_epoch(void)
{
    const auto ts = std::chrono::system_clock::now();
    return std::chrono::time_point_cast<std::chrono::nanoseconds>(ts).time_since_epoch().count();
}

#define anysignal_sharemap_pack_field(in, out, name) \
    anysignal::sharemap_pack_field(in.name, out.name)

#define anysignal_sharemap_unpack_field(in, out, name) \
    anysignal::sharemap_unpack_field(in.name, out.name)

template <typename ObjectMap, typename SharemapField, std::enable_if_t<std::is_same_v<SharemapField, std::array<char, STRING_BUFFER_SIZE>>, bool> = true>
void anysignal_sharemap_from_object_map_field(const ObjectMap &in, const char *const name, SharemapField &out)
{
    if (auto it = in.find(name); it != in.end()) {
        const std::string &ref = it->second;
        std::strncpy(out.data(), ref.c_str(), out.max_size() - 1);
        out[out.max_size() - 1] = '\0';
    }
}

template <typename ObjectMap, typename SharemapField, std::enable_if_t<std::negation_v<std::is_same<SharemapField, std::array<char, STRING_BUFFER_SIZE>>>, bool> = false>
void anysignal_sharemap_from_object_map_field(const ObjectMap &in, const char *const name, SharemapField &out)
{
    if (auto it = in.find(name); it != in.end()) {
        out = it->second;
    }
}

template <typename SharemapField, typename ObjectMap, std::enable_if_t<std::is_same_v<SharemapField, std::array<char, STRING_BUFFER_SIZE>>, bool> = true>
void anysignal_sharemap_to_object_map_field(const SharemapField &in, ObjectMap &out, const char *const name)
{
    out[name] = in.data();
}

template <typename SharemapField, typename ObjectMap, std::enable_if_t<std::negation_v<std::is_same<SharemapField, std::array<char, STRING_BUFFER_SIZE>>>, bool> = false>
void anysignal_sharemap_to_object_map_field(const SharemapField &in, ObjectMap &out, const char *const name)
{
    out[name] = in;
}

// config sharemap binary over the wire format
struct sharemap_config_packed_t
{
    std::uint8_t source_id[2]{};
    std::uint8_t schema_hash[8]{};
    std::uint8_t unix_timestamp_ns[8]{};
    std::uint8_t psk_cc_tx_force_on[1]{};
    std::uint8_t psk_cc_tx_idle_timeout_s[8]{};
    std::uint8_t psk_cc_tx_fe_frequency[8]{};
    std::uint8_t psk_cc_tx_fe_stx1_enable[1]{};
    std::uint8_t psk_cc_tx_fe_stx1_gain[8]{};
    std::uint8_t psk_cc_tx_fe_stx1_atten[8]{};
    std::uint8_t psk_cc_tx_fe_stx2_enable[1]{};
    std::uint8_t psk_cc_tx_fe_stx2_gain[8]{};
    std::uint8_t psk_cc_tx_fe_stx2_atten[8]{};
    std::uint8_t psk_cc_tx_fe_sample_rate[8]{};
    std::uint8_t psk_cc_tx_symbol_rate[8]{};
    std::uint8_t psk_cc_tx_modulation[64]{};
    std::uint8_t psk_cc_rx_force_on[1]{};
    std::uint8_t psk_cc_rx_idle_timeout_s[8]{};
    std::uint8_t psk_cc_rx_low_power_timeout_s[8]{};
    std::uint8_t psk_cc_rx_gain_mode[64]{};
    std::uint8_t psk_cc_rx_auto_antenna_selection[1]{};
    std::uint8_t psk_cc_rx_fe_frequency[8]{};
    std::uint8_t psk_cc_rx_fe_srx1_enable[1]{};
    std::uint8_t psk_cc_rx_fe_srx1_gain[8]{};
    std::uint8_t psk_cc_rx_fe_srx1_atten[8]{};
    std::uint8_t psk_cc_rx_fe_srx2_enable[1]{};
    std::uint8_t psk_cc_rx_fe_srx2_gain[8]{};
    std::uint8_t psk_cc_rx_fe_srx2_atten[8]{};
    std::uint8_t psk_cc_rx_fe_sample_rate[8]{};
    std::uint8_t psk_cc_rx_symbol_rate[8]{};
    std::uint8_t psk_cc_rx_modulation[64]{};
    std::uint8_t dvbs2_tx_force_on[1]{};
    std::uint8_t dvbs2_tx_idle_timeout_s[8]{};
    std::uint8_t dvbs2_tx_fe_frequency[8]{};
    std::uint8_t dvbs2_tx_fe_gain[8]{};
    std::uint8_t dvbs2_tx_fe_sample_rate[8]{};
    std::uint8_t dvbs2_tx_symbol_rate[8]{};
    std::uint8_t dvbs2_tx_modulation[64]{};
    std::uint8_t dvbs2_tx_coding[64]{};
    std::uint8_t dvbs2_tx_rolloff[64]{};
    std::uint8_t dvbs2_tx_frame_length[64]{};
    std::uint8_t dvbs2_tx_signal_scaling[8]{};
    std::uint8_t gfsk_tx_force_on[1]{};
    std::uint8_t gfsk_tx_idle_timeout_s[8]{};
    std::uint8_t gfsk_tx_fe_frequency[8]{};
    std::uint8_t gfsk_tx_fe_gain[8]{};
    std::uint8_t gfsk_tx_fe_atten[8]{};
    std::uint8_t gfsk_tx_fe_sample_rate[8]{};
    std::uint8_t gfsk_tx_symbol_rate[8]{};
    std::uint8_t gfsk_tx_mod_index[4]{};
    std::uint8_t gfsk_tx_max_payload_len[4]{};
    std::uint8_t gfsk_tx_bt[4]{};
    std::uint8_t anylink_active_tx_channel[64]{};
} __attribute__((packed));

struct sharemap_config_t
{
    static constexpr std::string_view NAME{"config"};
    static constexpr std::uint64_t HASH{0xa20b7ede39c02e9e};
    using packed_t = sharemap_config_packed_t;
    static constexpr size_t PACKED_SIZE{sizeof(packed_t)};
    
    // id of where the data comes from
    std::uint16_t source_id{};
    
    // hash of the schema used to ensure compatibility
    std::uint64_t schema_hash{HASH};
    
    // timestamp that counts the amount of time (in nanoseconds) since the unix epoch
    std::int64_t unix_timestamp_ns{time_ns_since_epoch()};
    
    // Force the channel to always be on.
    bool psk_cc_tx_force_on{};
    
    // Power down channel after being idle for specified time.
    std::uint64_t psk_cc_tx_idle_timeout_s{};
    
    // Frequency to transmit at.
    double psk_cc_tx_fe_frequency{};
    
    // Enable STX1 channel (STX2 must be disabled).
    bool psk_cc_tx_fe_stx1_enable{};
    
    // Gain setting for STX1.
    double psk_cc_tx_fe_stx1_gain{};
    
    // Digital step attenuator setting for STX1.
    double psk_cc_tx_fe_stx1_atten{};
    
    // Enable STX2 channel (STX1 must be disabled).
    bool psk_cc_tx_fe_stx2_enable{};
    
    // Gain setting for STX2.
    double psk_cc_tx_fe_stx2_gain{};
    
    // Digital step attenuator setting for STX2.
    double psk_cc_tx_fe_stx2_atten{};
    
    // Sample rate of the ad9361. The sample rate for all channels using the ad9361 should match if they are in active use (i.e. psk_cc tx/rx and dvbs2).
    double psk_cc_tx_fe_sample_rate{};
    
    // Symbol rate of the waveform.
    double psk_cc_tx_symbol_rate{};
    
    // Symbol modulation config: BPSK/QPSK
    std::array<char, STRING_BUFFER_SIZE> psk_cc_tx_modulation{};
    
    // Force the channel to always be on.
    bool psk_cc_rx_force_on{};
    
    // Power down channel after being idle for specified time.
    std::uint64_t psk_cc_rx_idle_timeout_s{};
    
    // Power up channel after being in powered down for specified time.
    std::uint64_t psk_cc_rx_low_power_timeout_s{};
    
    // Gain mode. Valid values are: MANUAL, SLOW_AGC, FAST_AGC, and HYBRID_AGC.
    std::array<char, STRING_BUFFER_SIZE> psk_cc_rx_gain_mode{};
    
    // Enable automatic antenna selection.
    bool psk_cc_rx_auto_antenna_selection{};
    
    // Frequency to receive from.
    double psk_cc_rx_fe_frequency{};
    
    // Enable SRX1 channel (SRX2 must be disabled).
    bool psk_cc_rx_fe_srx1_enable{};
    
    // Gain setting for SRX1.
    double psk_cc_rx_fe_srx1_gain{};
    
    // Digitial step attenuator setting for SRX1.
    double psk_cc_rx_fe_srx1_atten{};
    
    // Enable SRX2 channel (SRX1 must be disabled).
    bool psk_cc_rx_fe_srx2_enable{};
    
    // Gain setting for SRX2.
    double psk_cc_rx_fe_srx2_gain{};
    
    // Digital step attenuator setting for SRX2.
    double psk_cc_rx_fe_srx2_atten{};
    
    // Sample rate of the ad9361. The sample rate for all channels using the ad9361 should match if they are in active use (i.e. psk_cc tx/rx and dvbs2).
    double psk_cc_rx_fe_sample_rate{};
    
    // Symbol rate of the waveform.
    double psk_cc_rx_symbol_rate{};
    
    // Symbol modulation config: BPSK/QPSK
    std::array<char, STRING_BUFFER_SIZE> psk_cc_rx_modulation{};
    
    // Force the channel to always be on.
    bool dvbs2_tx_force_on{};
    
    // Power down channel after being idle for specified time.
    std::uint64_t dvbs2_tx_idle_timeout_s{};
    
    // Frequency to transmit at.
    double dvbs2_tx_fe_frequency{};
    
    // Gain setting for dvbs2 TX.
    double dvbs2_tx_fe_gain{};
    
    // Sample rate of the ad9361. The sample rate for all channels using the ad9361 should match if they are in active use (i.e. psk_cc tx/rx and dvbs2).
    double dvbs2_tx_fe_sample_rate{};
    
    // Symbol rate.  Must be integer division of sample rate.
    double dvbs2_tx_symbol_rate{};
    
    // Modulation to use. Valid values are: unmodulated, QPSK, 8PSK, 16APSK, and 32APSK.
    std::array<char, STRING_BUFFER_SIZE> dvbs2_tx_modulation{};
    
    // Error correction code to use. Valid values are: 1/4, 1/3, 2/5, 1/2, 3/5, 2/3, 3/4, 4/5, 5/6, 8/9, 9/10, 11/45, 4/15, 14/45, 7/15, 8/15, 26/45, and 32/45.
    std::array<char, STRING_BUFFER_SIZE> dvbs2_tx_coding{};
    
    // Filter rolloff. Valid values are: 35%, 25%, 20%, 15%, 10%, 5%, and the empty string for no rolloff.
    std::array<char, STRING_BUFFER_SIZE> dvbs2_tx_rolloff{};
    
    // Frame length type to use. Valid values are: SHORT, NORMAL, and LONG.
    std::array<char, STRING_BUFFER_SIZE> dvbs2_tx_frame_length{};
    
    // Scale of resulting signal.
    double dvbs2_tx_signal_scaling{};
    
    // Force the channel to always be on.
    bool gfsk_tx_force_on{};
    
    // Power down channel after being idle for specified time.
    std::uint64_t gfsk_tx_idle_timeout_s{};
    
    // Frequency to receive from.
    double gfsk_tx_fe_frequency{};
    
    // Gain setting for gfsk TX.
    double gfsk_tx_fe_gain{};
    
    // Digitial step attenuator setting for gfsk TX.
    double gfsk_tx_fe_atten{};
    
    // Sample rate of the rfic.
    double gfsk_tx_fe_sample_rate{};
    
    // Symbol rate.  Must be integer division of sample rate.
    double gfsk_tx_symbol_rate{};
    
    // Modulation index.
    float gfsk_tx_mod_index{};
    
    // Maximum payload length in bytes.
    std::uint32_t gfsk_tx_max_payload_len{};
    
    // 3db bandwidth symbol time product.
    float gfsk_tx_bt{};
    
    // The channel anylink should be actively downlinking on. Valid values are: tx_uhf, tx_sband, tx_xband. You can also use an empty string to disable the active channel.
    std::array<char, STRING_BUFFER_SIZE> anylink_active_tx_channel{};
    

    template <typename ObjectMap>
    void from_object_map(const ObjectMap &in)
    {
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("source_id"), this->source_id);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("schema_hash"), this->schema_hash);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("unix_timestamp_ns"), this->unix_timestamp_ns);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_tx_force_on"), this->psk_cc_tx_force_on);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_tx_idle_timeout_s"), this->psk_cc_tx_idle_timeout_s);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_tx_fe_frequency"), this->psk_cc_tx_fe_frequency);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_tx_fe_stx1_enable"), this->psk_cc_tx_fe_stx1_enable);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_tx_fe_stx1_gain"), this->psk_cc_tx_fe_stx1_gain);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_tx_fe_stx1_atten"), this->psk_cc_tx_fe_stx1_atten);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_tx_fe_stx2_enable"), this->psk_cc_tx_fe_stx2_enable);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_tx_fe_stx2_gain"), this->psk_cc_tx_fe_stx2_gain);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_tx_fe_stx2_atten"), this->psk_cc_tx_fe_stx2_atten);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_tx_fe_sample_rate"), this->psk_cc_tx_fe_sample_rate);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_tx_symbol_rate"), this->psk_cc_tx_symbol_rate);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_tx_modulation"), this->psk_cc_tx_modulation);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_rx_force_on"), this->psk_cc_rx_force_on);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_rx_idle_timeout_s"), this->psk_cc_rx_idle_timeout_s);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_rx_low_power_timeout_s"), this->psk_cc_rx_low_power_timeout_s);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_rx_gain_mode"), this->psk_cc_rx_gain_mode);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_rx_auto_antenna_selection"), this->psk_cc_rx_auto_antenna_selection);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_rx_fe_frequency"), this->psk_cc_rx_fe_frequency);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_rx_fe_srx1_enable"), this->psk_cc_rx_fe_srx1_enable);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_rx_fe_srx1_gain"), this->psk_cc_rx_fe_srx1_gain);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_rx_fe_srx1_atten"), this->psk_cc_rx_fe_srx1_atten);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_rx_fe_srx2_enable"), this->psk_cc_rx_fe_srx2_enable);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_rx_fe_srx2_gain"), this->psk_cc_rx_fe_srx2_gain);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_rx_fe_srx2_atten"), this->psk_cc_rx_fe_srx2_atten);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_rx_fe_sample_rate"), this->psk_cc_rx_fe_sample_rate);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_rx_symbol_rate"), this->psk_cc_rx_symbol_rate);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_rx_modulation"), this->psk_cc_rx_modulation);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("dvbs2_tx_force_on"), this->dvbs2_tx_force_on);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("dvbs2_tx_idle_timeout_s"), this->dvbs2_tx_idle_timeout_s);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("dvbs2_tx_fe_frequency"), this->dvbs2_tx_fe_frequency);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("dvbs2_tx_fe_gain"), this->dvbs2_tx_fe_gain);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("dvbs2_tx_fe_sample_rate"), this->dvbs2_tx_fe_sample_rate);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("dvbs2_tx_symbol_rate"), this->dvbs2_tx_symbol_rate);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("dvbs2_tx_modulation"), this->dvbs2_tx_modulation);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("dvbs2_tx_coding"), this->dvbs2_tx_coding);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("dvbs2_tx_rolloff"), this->dvbs2_tx_rolloff);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("dvbs2_tx_frame_length"), this->dvbs2_tx_frame_length);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("dvbs2_tx_signal_scaling"), this->dvbs2_tx_signal_scaling);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("gfsk_tx_force_on"), this->gfsk_tx_force_on);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("gfsk_tx_idle_timeout_s"), this->gfsk_tx_idle_timeout_s);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("gfsk_tx_fe_frequency"), this->gfsk_tx_fe_frequency);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("gfsk_tx_fe_gain"), this->gfsk_tx_fe_gain);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("gfsk_tx_fe_atten"), this->gfsk_tx_fe_atten);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("gfsk_tx_fe_sample_rate"), this->gfsk_tx_fe_sample_rate);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("gfsk_tx_symbol_rate"), this->gfsk_tx_symbol_rate);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("gfsk_tx_mod_index"), this->gfsk_tx_mod_index);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("gfsk_tx_max_payload_len"), this->gfsk_tx_max_payload_len);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("gfsk_tx_bt"), this->gfsk_tx_bt);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_active_tx_channel"), this->anylink_active_tx_channel);
    }

    template <typename ObjectMap>
    void to_object_map(ObjectMap &out) const
    {
        anysignal_sharemap_to_object_map_field(this->source_id, out, reinterpret_cast<const char *>("source_id"));
        anysignal_sharemap_to_object_map_field(this->schema_hash, out, reinterpret_cast<const char *>("schema_hash"));
        anysignal_sharemap_to_object_map_field(this->unix_timestamp_ns, out, reinterpret_cast<const char *>("unix_timestamp_ns"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_tx_force_on, out, reinterpret_cast<const char *>("psk_cc_tx_force_on"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_tx_idle_timeout_s, out, reinterpret_cast<const char *>("psk_cc_tx_idle_timeout_s"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_tx_fe_frequency, out, reinterpret_cast<const char *>("psk_cc_tx_fe_frequency"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_tx_fe_stx1_enable, out, reinterpret_cast<const char *>("psk_cc_tx_fe_stx1_enable"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_tx_fe_stx1_gain, out, reinterpret_cast<const char *>("psk_cc_tx_fe_stx1_gain"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_tx_fe_stx1_atten, out, reinterpret_cast<const char *>("psk_cc_tx_fe_stx1_atten"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_tx_fe_stx2_enable, out, reinterpret_cast<const char *>("psk_cc_tx_fe_stx2_enable"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_tx_fe_stx2_gain, out, reinterpret_cast<const char *>("psk_cc_tx_fe_stx2_gain"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_tx_fe_stx2_atten, out, reinterpret_cast<const char *>("psk_cc_tx_fe_stx2_atten"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_tx_fe_sample_rate, out, reinterpret_cast<const char *>("psk_cc_tx_fe_sample_rate"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_tx_symbol_rate, out, reinterpret_cast<const char *>("psk_cc_tx_symbol_rate"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_tx_modulation, out, reinterpret_cast<const char *>("psk_cc_tx_modulation"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_rx_force_on, out, reinterpret_cast<const char *>("psk_cc_rx_force_on"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_rx_idle_timeout_s, out, reinterpret_cast<const char *>("psk_cc_rx_idle_timeout_s"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_rx_low_power_timeout_s, out, reinterpret_cast<const char *>("psk_cc_rx_low_power_timeout_s"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_rx_gain_mode, out, reinterpret_cast<const char *>("psk_cc_rx_gain_mode"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_rx_auto_antenna_selection, out, reinterpret_cast<const char *>("psk_cc_rx_auto_antenna_selection"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_rx_fe_frequency, out, reinterpret_cast<const char *>("psk_cc_rx_fe_frequency"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_rx_fe_srx1_enable, out, reinterpret_cast<const char *>("psk_cc_rx_fe_srx1_enable"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_rx_fe_srx1_gain, out, reinterpret_cast<const char *>("psk_cc_rx_fe_srx1_gain"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_rx_fe_srx1_atten, out, reinterpret_cast<const char *>("psk_cc_rx_fe_srx1_atten"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_rx_fe_srx2_enable, out, reinterpret_cast<const char *>("psk_cc_rx_fe_srx2_enable"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_rx_fe_srx2_gain, out, reinterpret_cast<const char *>("psk_cc_rx_fe_srx2_gain"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_rx_fe_srx2_atten, out, reinterpret_cast<const char *>("psk_cc_rx_fe_srx2_atten"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_rx_fe_sample_rate, out, reinterpret_cast<const char *>("psk_cc_rx_fe_sample_rate"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_rx_symbol_rate, out, reinterpret_cast<const char *>("psk_cc_rx_symbol_rate"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_rx_modulation, out, reinterpret_cast<const char *>("psk_cc_rx_modulation"));
        anysignal_sharemap_to_object_map_field(this->dvbs2_tx_force_on, out, reinterpret_cast<const char *>("dvbs2_tx_force_on"));
        anysignal_sharemap_to_object_map_field(this->dvbs2_tx_idle_timeout_s, out, reinterpret_cast<const char *>("dvbs2_tx_idle_timeout_s"));
        anysignal_sharemap_to_object_map_field(this->dvbs2_tx_fe_frequency, out, reinterpret_cast<const char *>("dvbs2_tx_fe_frequency"));
        anysignal_sharemap_to_object_map_field(this->dvbs2_tx_fe_gain, out, reinterpret_cast<const char *>("dvbs2_tx_fe_gain"));
        anysignal_sharemap_to_object_map_field(this->dvbs2_tx_fe_sample_rate, out, reinterpret_cast<const char *>("dvbs2_tx_fe_sample_rate"));
        anysignal_sharemap_to_object_map_field(this->dvbs2_tx_symbol_rate, out, reinterpret_cast<const char *>("dvbs2_tx_symbol_rate"));
        anysignal_sharemap_to_object_map_field(this->dvbs2_tx_modulation, out, reinterpret_cast<const char *>("dvbs2_tx_modulation"));
        anysignal_sharemap_to_object_map_field(this->dvbs2_tx_coding, out, reinterpret_cast<const char *>("dvbs2_tx_coding"));
        anysignal_sharemap_to_object_map_field(this->dvbs2_tx_rolloff, out, reinterpret_cast<const char *>("dvbs2_tx_rolloff"));
        anysignal_sharemap_to_object_map_field(this->dvbs2_tx_frame_length, out, reinterpret_cast<const char *>("dvbs2_tx_frame_length"));
        anysignal_sharemap_to_object_map_field(this->dvbs2_tx_signal_scaling, out, reinterpret_cast<const char *>("dvbs2_tx_signal_scaling"));
        anysignal_sharemap_to_object_map_field(this->gfsk_tx_force_on, out, reinterpret_cast<const char *>("gfsk_tx_force_on"));
        anysignal_sharemap_to_object_map_field(this->gfsk_tx_idle_timeout_s, out, reinterpret_cast<const char *>("gfsk_tx_idle_timeout_s"));
        anysignal_sharemap_to_object_map_field(this->gfsk_tx_fe_frequency, out, reinterpret_cast<const char *>("gfsk_tx_fe_frequency"));
        anysignal_sharemap_to_object_map_field(this->gfsk_tx_fe_gain, out, reinterpret_cast<const char *>("gfsk_tx_fe_gain"));
        anysignal_sharemap_to_object_map_field(this->gfsk_tx_fe_atten, out, reinterpret_cast<const char *>("gfsk_tx_fe_atten"));
        anysignal_sharemap_to_object_map_field(this->gfsk_tx_fe_sample_rate, out, reinterpret_cast<const char *>("gfsk_tx_fe_sample_rate"));
        anysignal_sharemap_to_object_map_field(this->gfsk_tx_symbol_rate, out, reinterpret_cast<const char *>("gfsk_tx_symbol_rate"));
        anysignal_sharemap_to_object_map_field(this->gfsk_tx_mod_index, out, reinterpret_cast<const char *>("gfsk_tx_mod_index"));
        anysignal_sharemap_to_object_map_field(this->gfsk_tx_max_payload_len, out, reinterpret_cast<const char *>("gfsk_tx_max_payload_len"));
        anysignal_sharemap_to_object_map_field(this->gfsk_tx_bt, out, reinterpret_cast<const char *>("gfsk_tx_bt"));
        anysignal_sharemap_to_object_map_field(this->anylink_active_tx_channel, out, reinterpret_cast<const char *>("anylink_active_tx_channel"));
    }
};

static inline sharemap_config_packed_t sharemap_pack(sharemap_config_t &in)
{
    in.unix_timestamp_ns = time_ns_since_epoch();
    sharemap_config_packed_t out{};
    anysignal_sharemap_pack_field(in, out, source_id);
    anysignal_sharemap_pack_field(in, out, schema_hash);
    anysignal_sharemap_pack_field(in, out, unix_timestamp_ns);
    anysignal_sharemap_pack_field(in, out, psk_cc_tx_force_on);
    anysignal_sharemap_pack_field(in, out, psk_cc_tx_idle_timeout_s);
    anysignal_sharemap_pack_field(in, out, psk_cc_tx_fe_frequency);
    anysignal_sharemap_pack_field(in, out, psk_cc_tx_fe_stx1_enable);
    anysignal_sharemap_pack_field(in, out, psk_cc_tx_fe_stx1_gain);
    anysignal_sharemap_pack_field(in, out, psk_cc_tx_fe_stx1_atten);
    anysignal_sharemap_pack_field(in, out, psk_cc_tx_fe_stx2_enable);
    anysignal_sharemap_pack_field(in, out, psk_cc_tx_fe_stx2_gain);
    anysignal_sharemap_pack_field(in, out, psk_cc_tx_fe_stx2_atten);
    anysignal_sharemap_pack_field(in, out, psk_cc_tx_fe_sample_rate);
    anysignal_sharemap_pack_field(in, out, psk_cc_tx_symbol_rate);
    anysignal_sharemap_pack_field(in, out, psk_cc_tx_modulation);
    anysignal_sharemap_pack_field(in, out, psk_cc_rx_force_on);
    anysignal_sharemap_pack_field(in, out, psk_cc_rx_idle_timeout_s);
    anysignal_sharemap_pack_field(in, out, psk_cc_rx_low_power_timeout_s);
    anysignal_sharemap_pack_field(in, out, psk_cc_rx_gain_mode);
    anysignal_sharemap_pack_field(in, out, psk_cc_rx_auto_antenna_selection);
    anysignal_sharemap_pack_field(in, out, psk_cc_rx_fe_frequency);
    anysignal_sharemap_pack_field(in, out, psk_cc_rx_fe_srx1_enable);
    anysignal_sharemap_pack_field(in, out, psk_cc_rx_fe_srx1_gain);
    anysignal_sharemap_pack_field(in, out, psk_cc_rx_fe_srx1_atten);
    anysignal_sharemap_pack_field(in, out, psk_cc_rx_fe_srx2_enable);
    anysignal_sharemap_pack_field(in, out, psk_cc_rx_fe_srx2_gain);
    anysignal_sharemap_pack_field(in, out, psk_cc_rx_fe_srx2_atten);
    anysignal_sharemap_pack_field(in, out, psk_cc_rx_fe_sample_rate);
    anysignal_sharemap_pack_field(in, out, psk_cc_rx_symbol_rate);
    anysignal_sharemap_pack_field(in, out, psk_cc_rx_modulation);
    anysignal_sharemap_pack_field(in, out, dvbs2_tx_force_on);
    anysignal_sharemap_pack_field(in, out, dvbs2_tx_idle_timeout_s);
    anysignal_sharemap_pack_field(in, out, dvbs2_tx_fe_frequency);
    anysignal_sharemap_pack_field(in, out, dvbs2_tx_fe_gain);
    anysignal_sharemap_pack_field(in, out, dvbs2_tx_fe_sample_rate);
    anysignal_sharemap_pack_field(in, out, dvbs2_tx_symbol_rate);
    anysignal_sharemap_pack_field(in, out, dvbs2_tx_modulation);
    anysignal_sharemap_pack_field(in, out, dvbs2_tx_coding);
    anysignal_sharemap_pack_field(in, out, dvbs2_tx_rolloff);
    anysignal_sharemap_pack_field(in, out, dvbs2_tx_frame_length);
    anysignal_sharemap_pack_field(in, out, dvbs2_tx_signal_scaling);
    anysignal_sharemap_pack_field(in, out, gfsk_tx_force_on);
    anysignal_sharemap_pack_field(in, out, gfsk_tx_idle_timeout_s);
    anysignal_sharemap_pack_field(in, out, gfsk_tx_fe_frequency);
    anysignal_sharemap_pack_field(in, out, gfsk_tx_fe_gain);
    anysignal_sharemap_pack_field(in, out, gfsk_tx_fe_atten);
    anysignal_sharemap_pack_field(in, out, gfsk_tx_fe_sample_rate);
    anysignal_sharemap_pack_field(in, out, gfsk_tx_symbol_rate);
    anysignal_sharemap_pack_field(in, out, gfsk_tx_mod_index);
    anysignal_sharemap_pack_field(in, out, gfsk_tx_max_payload_len);
    anysignal_sharemap_pack_field(in, out, gfsk_tx_bt);
    anysignal_sharemap_pack_field(in, out, anylink_active_tx_channel);
    return out;
}

static inline sharemap_config_t sharemap_unpack(const sharemap_config_packed_t &in)
{
    sharemap_config_t out{};
    anysignal_sharemap_unpack_field(in, out, source_id);
    anysignal_sharemap_unpack_field(in, out, schema_hash);
    anysignal_sharemap_unpack_field(in, out, unix_timestamp_ns);
    anysignal_sharemap_unpack_field(in, out, psk_cc_tx_force_on);
    anysignal_sharemap_unpack_field(in, out, psk_cc_tx_idle_timeout_s);
    anysignal_sharemap_unpack_field(in, out, psk_cc_tx_fe_frequency);
    anysignal_sharemap_unpack_field(in, out, psk_cc_tx_fe_stx1_enable);
    anysignal_sharemap_unpack_field(in, out, psk_cc_tx_fe_stx1_gain);
    anysignal_sharemap_unpack_field(in, out, psk_cc_tx_fe_stx1_atten);
    anysignal_sharemap_unpack_field(in, out, psk_cc_tx_fe_stx2_enable);
    anysignal_sharemap_unpack_field(in, out, psk_cc_tx_fe_stx2_gain);
    anysignal_sharemap_unpack_field(in, out, psk_cc_tx_fe_stx2_atten);
    anysignal_sharemap_unpack_field(in, out, psk_cc_tx_fe_sample_rate);
    anysignal_sharemap_unpack_field(in, out, psk_cc_tx_symbol_rate);
    anysignal_sharemap_unpack_field(in, out, psk_cc_tx_modulation);
    anysignal_sharemap_unpack_field(in, out, psk_cc_rx_force_on);
    anysignal_sharemap_unpack_field(in, out, psk_cc_rx_idle_timeout_s);
    anysignal_sharemap_unpack_field(in, out, psk_cc_rx_low_power_timeout_s);
    anysignal_sharemap_unpack_field(in, out, psk_cc_rx_gain_mode);
    anysignal_sharemap_unpack_field(in, out, psk_cc_rx_auto_antenna_selection);
    anysignal_sharemap_unpack_field(in, out, psk_cc_rx_fe_frequency);
    anysignal_sharemap_unpack_field(in, out, psk_cc_rx_fe_srx1_enable);
    anysignal_sharemap_unpack_field(in, out, psk_cc_rx_fe_srx1_gain);
    anysignal_sharemap_unpack_field(in, out, psk_cc_rx_fe_srx1_atten);
    anysignal_sharemap_unpack_field(in, out, psk_cc_rx_fe_srx2_enable);
    anysignal_sharemap_unpack_field(in, out, psk_cc_rx_fe_srx2_gain);
    anysignal_sharemap_unpack_field(in, out, psk_cc_rx_fe_srx2_atten);
    anysignal_sharemap_unpack_field(in, out, psk_cc_rx_fe_sample_rate);
    anysignal_sharemap_unpack_field(in, out, psk_cc_rx_symbol_rate);
    anysignal_sharemap_unpack_field(in, out, psk_cc_rx_modulation);
    anysignal_sharemap_unpack_field(in, out, dvbs2_tx_force_on);
    anysignal_sharemap_unpack_field(in, out, dvbs2_tx_idle_timeout_s);
    anysignal_sharemap_unpack_field(in, out, dvbs2_tx_fe_frequency);
    anysignal_sharemap_unpack_field(in, out, dvbs2_tx_fe_gain);
    anysignal_sharemap_unpack_field(in, out, dvbs2_tx_fe_sample_rate);
    anysignal_sharemap_unpack_field(in, out, dvbs2_tx_symbol_rate);
    anysignal_sharemap_unpack_field(in, out, dvbs2_tx_modulation);
    anysignal_sharemap_unpack_field(in, out, dvbs2_tx_coding);
    anysignal_sharemap_unpack_field(in, out, dvbs2_tx_rolloff);
    anysignal_sharemap_unpack_field(in, out, dvbs2_tx_frame_length);
    anysignal_sharemap_unpack_field(in, out, dvbs2_tx_signal_scaling);
    anysignal_sharemap_unpack_field(in, out, gfsk_tx_force_on);
    anysignal_sharemap_unpack_field(in, out, gfsk_tx_idle_timeout_s);
    anysignal_sharemap_unpack_field(in, out, gfsk_tx_fe_frequency);
    anysignal_sharemap_unpack_field(in, out, gfsk_tx_fe_gain);
    anysignal_sharemap_unpack_field(in, out, gfsk_tx_fe_atten);
    anysignal_sharemap_unpack_field(in, out, gfsk_tx_fe_sample_rate);
    anysignal_sharemap_unpack_field(in, out, gfsk_tx_symbol_rate);
    anysignal_sharemap_unpack_field(in, out, gfsk_tx_mod_index);
    anysignal_sharemap_unpack_field(in, out, gfsk_tx_max_payload_len);
    anysignal_sharemap_unpack_field(in, out, gfsk_tx_bt);
    anysignal_sharemap_unpack_field(in, out, anylink_active_tx_channel);
    return out;
}

// metrics sharemap binary over the wire format
struct sharemap_metrics_packed_t
{
    std::uint8_t source_id[2]{};
    std::uint8_t schema_hash[8]{};
    std::uint8_t unix_timestamp_ns[8]{};
    std::uint8_t controld_version[64]{};
    std::uint8_t controld_timestamp[64]{};
    std::uint8_t powerd_version[64]{};
    std::uint8_t powerd_timestamp[64]{};
    std::uint8_t radiod_version[64]{};
    std::uint8_t radiod_timestamp[64]{};
    std::uint8_t fpga_version[64]{};
    std::uint8_t fpga_timestamp[64]{};
    std::uint8_t fpga_project_name[64]{};
    std::uint8_t anylink_version[64]{};
    std::uint8_t psk_cc_tx_bytes_total[8]{};
    std::uint8_t psk_cc_tx_underflows[8]{};
    std::uint8_t psk_cc_tx_client_recv_errors[8]{};
    std::uint8_t psk_cc_tx_client_msgs[8]{};
    std::uint8_t psk_cc_tx_frames_transmitted[8]{};
    std::uint8_t psk_cc_tx_failed_transmissions[8]{};
    std::uint8_t psk_cc_tx_dropped_packets[8]{};
    std::uint8_t psk_cc_tx_idle_frames_transmitted[8]{};
    std::uint8_t psk_cc_tx_failed_idle_frames_transmitted[8]{};
    std::uint8_t psk_cc_tx_failed_bytes_in_flight_checks[8]{};
    std::uint8_t psk_cc_tx_modem_underflows[8]{};
    std::uint8_t psk_cc_tx_ad9361_tx_pll_lock[1]{};
    std::uint8_t psk_cc_rx_bytes_total[8]{};
    std::uint8_t psk_cc_rx_client_send_errors[8]{};
    std::uint8_t psk_cc_rx_client_msgs[8]{};
    std::uint8_t psk_cc_rx_frames_received[8]{};
    std::uint8_t psk_cc_rx_failed_receptions[8]{};
    std::uint8_t psk_cc_rx_dropped_good_packets[8]{};
    std::uint8_t psk_cc_rx_failed_frames_available_checks[8]{};
    std::uint8_t psk_cc_rx_encountered_frames_in_progress[8]{};
    std::uint8_t psk_cc_rx_modem_dma_overflows[8]{};
    std::uint8_t psk_cc_rx_modem_dma_packet_count[4]{};
    std::uint8_t psk_cc_rx_signal_present[1]{};
    std::uint8_t psk_cc_rx_carrier_lock[1]{};
    std::uint8_t psk_cc_rx_frame_sync_lock[1]{};
    std::uint8_t psk_cc_rx_fec_confirmed_lock[1]{};
    std::uint8_t psk_cc_rx_fec_ber[4]{};
    std::uint8_t psk_cc_rx_ad9361_rx_pll_lock[1]{};
    std::uint8_t psk_cc_rx_ad9361_bb_pll_lock[1]{};
    std::uint8_t dvbs2_tx_bytes_total[8]{};
    std::uint8_t dvbs2_tx_underflows[8]{};
    std::uint8_t dvbs2_tx_client_recv_errors[8]{};
    std::uint8_t dvbs2_tx_client_msgs[8]{};
    std::uint8_t dvbs2_tx_frames_transmitted[8]{};
    std::uint8_t dvbs2_tx_failed_transmissions[8]{};
    std::uint8_t dvbs2_tx_dropped_packets[8]{};
    std::uint8_t dvbs2_tx_idle_frames_transmitted[8]{};
    std::uint8_t dvbs2_tx_failed_idle_frames_transmitted[8]{};
    std::uint8_t dvbs2_tx_failed_bytes_in_flight_checks[8]{};
    std::uint8_t dvbs2_tx_dummy_pl_frames[8]{};
    std::uint8_t gfsk_tx_bytes_total[8]{};
    std::uint8_t gfsk_tx_underflows[8]{};
    std::uint8_t gfsk_tx_client_recv_errors[8]{};
    std::uint8_t gfsk_tx_client_msgs[8]{};
    std::uint8_t gfsk_tx_frames_transmitted[8]{};
    std::uint8_t gfsk_tx_failed_transmissions[8]{};
    std::uint8_t gfsk_tx_dropped_packets[8]{};
    std::uint8_t gfsk_tx_idle_frames_transmitted[8]{};
    std::uint8_t gfsk_tx_failed_idle_frames_transmitted[8]{};
    std::uint8_t gfsk_tx_failed_bytes_in_flight_checks[8]{};
    std::uint8_t ad9122_pgood[1]{};
    std::uint8_t ad9361_pgood[1]{};
    std::uint8_t adrf6780_pgood[1]{};
    std::uint8_t at86_pgood[1]{};
    std::uint8_t at86_is_pll_locked[1]{};
    std::uint8_t aux_3v8_isense[8]{};
    std::uint8_t aux_3v8_vsense[8]{};
    std::uint8_t carrier_28v0_isense[8]{};
    std::uint8_t carrier_28v0_vsense[8]{};
    std::uint8_t carrier_2v1_isense[8]{};
    std::uint8_t carrier_2v1_vsense[8]{};
    std::uint8_t carrier_2v6_isense[8]{};
    std::uint8_t carrier_2v6_vsense[8]{};
    std::uint8_t carrier_3v8_isense[8]{};
    std::uint8_t carrier_3v8_vsense[8]{};
    std::uint8_t carrier_5v5_isense[8]{};
    std::uint8_t carrier_5v5_vsense[8]{};
    std::uint8_t carrier_temp[8]{};
    std::uint8_t lband_rx_pgood[1]{};
    std::uint8_t lband_temp[8]{};
    std::uint8_t lband_tx_pgood[1]{};
    std::uint8_t lband_tx_rf_detect[8]{};
    std::uint8_t lmk04832_pgood[1]{};
    std::uint8_t lmk04832_is_pll_locked[1]{};
    std::uint8_t lmx2594_pgood[1]{};
    std::uint8_t max2771_a_1_is_pll_locked[1]{};
    std::uint8_t max2771_a_2_is_pll_locked[1]{};
    std::uint8_t max2771_a_bias_pgood[1]{};
    std::uint8_t max2771_a_pgood[1]{};
    std::uint8_t max2771_b_1_is_pll_locked[1]{};
    std::uint8_t max2771_b_2_is_pll_locked[1]{};
    std::uint8_t max2771_b_bias_pgood[1]{};
    std::uint8_t max2771_b_pgood[1]{};
    std::uint8_t rf_fe_mux_pgood[1]{};
    std::uint8_t sband_rx_pgood[1]{};
    std::uint8_t sband_temp[8]{};
    std::uint8_t sband_tx_pgood[1]{};
    std::uint8_t sband_tx_rf_detect[8]{};
    std::uint8_t si5345_pgood[1]{};
    std::uint8_t som_5v0_isense[8]{};
    std::uint8_t som_5v0_vsense[8]{};
    std::uint8_t uhf_rx_pgood[1]{};
    std::uint8_t uhf_temp[8]{};
    std::uint8_t uhf_tx_pgood[1]{};
    std::uint8_t uhf_tx_rf_detect[8]{};
    std::uint8_t xband_24v0_isense[8]{};
    std::uint8_t xband_24v0_vsense[8]{};
    std::uint8_t xband_drain_pgood[1]{};
    std::uint8_t xband_temp[8]{};
    std::uint8_t xband_tx_rf_detect[8]{};
    std::uint8_t anylink_uhf_tx_sent_bytes[8]{};
    std::uint8_t anylink_uhf_tx_sent_packets[8]{};
    std::uint8_t anylink_uhf_tx_sent_frames[8]{};
    std::uint8_t anylink_uhf_tx_overflow_frames[8]{};
    std::uint8_t anylink_sband_tx_sent_bytes[8]{};
    std::uint8_t anylink_sband_tx_sent_packets[8]{};
    std::uint8_t anylink_sband_tx_sent_frames[8]{};
    std::uint8_t anylink_sband_tx_overflow_frames[8]{};
    std::uint8_t anylink_xband_tx_sent_bytes[8]{};
    std::uint8_t anylink_xband_tx_sent_packets[8]{};
    std::uint8_t anylink_xband_tx_sent_frames[8]{};
    std::uint8_t anylink_xband_tx_overflow_frames[8]{};
    std::uint8_t anylink_sband_rx_received_bytes[8]{};
    std::uint8_t anylink_sband_rx_received_packets[8]{};
    std::uint8_t anylink_sband_rx_received_frames[8]{};
    std::uint8_t anylink_sband_rx_dropped_packets[8]{};
    std::uint8_t anylink_sband_rx_dropped_frames[8]{};
    std::uint8_t anylink_sband_rx_socket_errors[8]{};
    std::uint8_t anylink_sband_rx_idle_frames[8]{};
    std::uint8_t anylink_heartbeats_sent[8]{};
    std::uint8_t anylink_heartbeats_received[8]{};
    std::uint8_t anylink_rx_radio_bad_header[8]{};
    std::uint8_t anylink_rx_radio_packets_received[8]{};
    std::uint8_t anylink_tx_radio_packets_send_errors[8]{};
    std::uint8_t anylink_tx_radio_packets_sent[8]{};
    std::uint8_t anylink_tx_radio_packet_nodest[8]{};
    std::uint8_t anylink_tx_radio_packet_truncate[8]{};
    std::uint8_t anylink_tx_radio_packet_pad[8]{};
    std::uint8_t anylink_rx_radio_no_endpoint[8]{};
    std::uint8_t anylink_rx_radio_reject_echo[8]{};
    std::uint8_t anylink_total_endpoint_packets_received[8]{};
    std::uint8_t anylink_total_endpoint_packets_sent[8]{};
    std::uint8_t anylink_encryption_failed[8]{};
    std::uint8_t anylink_decryption_failed[8]{};
    std::uint8_t anylink_tap_endpoint_active_tx_channel[64]{};
    std::uint8_t anylink_tap_endpoint_mtu[8]{};
    std::uint8_t anylink_tap_endpoint_recv_bytes[8]{};
    std::uint8_t anylink_tap_endpoint_recv_errors[8]{};
    std::uint8_t anylink_tap_endpoint_recv_packets[8]{};
    std::uint8_t anylink_tap_endpoint_send_bytes[8]{};
    std::uint8_t anylink_tap_endpoint_send_errors[8]{};
    std::uint8_t anylink_tap_endpoint_send_packets[8]{};
} __attribute__((packed));

struct sharemap_metrics_t
{
    static constexpr std::string_view NAME{"metrics"};
    static constexpr std::uint64_t HASH{0x3ec97e7957b3a184};
    using packed_t = sharemap_metrics_packed_t;
    static constexpr size_t PACKED_SIZE{sizeof(packed_t)};
    
    // id of where the data comes from
    std::uint16_t source_id{};
    
    // hash of the schema used to ensure compatibility
    std::uint64_t schema_hash{HASH};
    
    // timestamp that counts the amount of time (in nanoseconds) since the unix epoch
    std::int64_t unix_timestamp_ns{time_ns_since_epoch()};
    
    // The version of controld
    std::array<char, STRING_BUFFER_SIZE> controld_version{};
    
    // The timestamp of the powerd build
    std::array<char, STRING_BUFFER_SIZE> controld_timestamp{};
    
    // The version of powerd
    std::array<char, STRING_BUFFER_SIZE> powerd_version{};
    
    // The timestamp of the powerd build
    std::array<char, STRING_BUFFER_SIZE> powerd_timestamp{};
    
    // The version of radiod
    std::array<char, STRING_BUFFER_SIZE> radiod_version{};
    
    // The timestamp of the radiod build
    std::array<char, STRING_BUFFER_SIZE> radiod_timestamp{};
    
    // The version of the fpga
    std::array<char, STRING_BUFFER_SIZE> fpga_version{};
    
    // The timestamp of the fpga build
    std::array<char, STRING_BUFFER_SIZE> fpga_timestamp{};
    
    // The name of the fpga project
    std::array<char, STRING_BUFFER_SIZE> fpga_project_name{};
    
    // The version of anylink
    std::array<char, STRING_BUFFER_SIZE> anylink_version{};
    
    // The number of bytes we have received from the tx socket that successfully sent.
    std::uint64_t psk_cc_tx_bytes_total{};
    
    // The number of times we've underflowed.
    std::uint64_t psk_cc_tx_underflows{};
    
    // Every time we get a bad return value from recv'ing on the tx socket.
    std::uint64_t psk_cc_tx_client_recv_errors{};
    
    // Every time we successfully recv'd on the tx socket.
    std::uint64_t psk_cc_tx_client_msgs{};
    
    // Every time we were able to transmit a frame over rf.
    std::uint64_t psk_cc_tx_frames_transmitted{};
    
    // Every time we were unable to transmit a frame over rf.
    std::uint64_t psk_cc_tx_failed_transmissions{};
    
    // Every time a packet is dropped due to failure to enable a channel.
    std::uint64_t psk_cc_tx_dropped_packets{};
    
    // The total number of idle frames transmitted.
    std::uint64_t psk_cc_tx_idle_frames_transmitted{};
    
    // The amount of times we tried to transmit an idle frame and it failed
    std::uint64_t psk_cc_tx_failed_idle_frames_transmitted{};
    
    // The amount of times the check for bytes_in_flight failed.
    std::uint64_t psk_cc_tx_failed_bytes_in_flight_checks{};
    
    // The number of times we've underflowed (as detected by the modem).
    std::uint64_t psk_cc_tx_modem_underflows{};
    
    // Is the tx pll of the ad9361 locked?
    bool psk_cc_tx_ad9361_tx_pll_lock{};
    
    // The number of bytes we have received and communicated to the client.
    std::uint64_t psk_cc_rx_bytes_total{};
    
    // Every time we get a bad return value from send'ing on the rx socket
    std::uint64_t psk_cc_rx_client_send_errors{};
    
    // Every time we successfully send on the rx socket.
    std::uint64_t psk_cc_rx_client_msgs{};
    
    // Every time we were able to receive a frame over rf.
    std::uint64_t psk_cc_rx_frames_received{};
    
    // Every time we were unable to receive a frame over rf.
    std::uint64_t psk_cc_rx_failed_receptions{};
    
    // Every time the socket's queue is full and we have to drop a good packet.
    std::uint64_t psk_cc_rx_dropped_good_packets{};
    
    // The amount of times the check for frames_available failed.
    std::uint64_t psk_cc_rx_failed_frames_available_checks{};
    
    // The amount of times we encountered frames in progress when checking for the number of frames available.
    std::uint64_t psk_cc_rx_encountered_frames_in_progress{};
    
    // The amount of times the modem overflows.
    std::uint64_t psk_cc_rx_modem_dma_overflows{};
    
    // The number of packets in the DMA.
    std::uint32_t psk_cc_rx_modem_dma_packet_count{};
    
    // Does the modem detect if a signal is present?
    bool psk_cc_rx_signal_present{};
    
    // Is the modem locked on to the carrier?
    bool psk_cc_rx_carrier_lock{};
    
    // Are we seeing frame sync words in the modem?
    bool psk_cc_rx_frame_sync_lock{};
    
    // FEC lock status
    bool psk_cc_rx_fec_confirmed_lock{};
    
    // FEC BER
    float psk_cc_rx_fec_ber{};
    
    // Is the rx pll of the ad9361 locked?
    bool psk_cc_rx_ad9361_rx_pll_lock{};
    
    // Is the baseband pll locked? It’s used to generate all baseband related clock signals.
    bool psk_cc_rx_ad9361_bb_pll_lock{};
    
    // The number of bytes we have received from the tx socket that successfully sent.
    std::uint64_t dvbs2_tx_bytes_total{};
    
    // The number of times we've underflowed.
    std::uint64_t dvbs2_tx_underflows{};
    
    // Every time we get a bad return value from recv'ing on the tx socket.
    std::uint64_t dvbs2_tx_client_recv_errors{};
    
    // Every time we successfully recv'd on the tx socket.
    std::uint64_t dvbs2_tx_client_msgs{};
    
    // Every time we were able to transmit a frame over rf.
    std::uint64_t dvbs2_tx_frames_transmitted{};
    
    // Every time we were unable to transmit a frame over rf.
    std::uint64_t dvbs2_tx_failed_transmissions{};
    
    // Every time a packet is dropped due to failure to enable a channel.
    std::uint64_t dvbs2_tx_dropped_packets{};
    
    // The total number of idle frames transmitted.
    std::uint64_t dvbs2_tx_idle_frames_transmitted{};
    
    // The amount of times we tried to transmit an idle frame and it failed
    std::uint64_t dvbs2_tx_failed_idle_frames_transmitted{};
    
    // The amount of times the check for bytes_in_flight failed.
    std::uint64_t dvbs2_tx_failed_bytes_in_flight_checks{};
    
    // The number of dummy pl frames sent by the modem.
    std::uint64_t dvbs2_tx_dummy_pl_frames{};
    
    // The number of bytes we have received from the tx socket that successfully sent.
    std::uint64_t gfsk_tx_bytes_total{};
    
    // The number of times we've underflowed.
    std::uint64_t gfsk_tx_underflows{};
    
    // Every time we get a bad return value from recv'ing on the tx socket.
    std::uint64_t gfsk_tx_client_recv_errors{};
    
    // Every time we successfully recv'd on the tx socket.
    std::uint64_t gfsk_tx_client_msgs{};
    
    // Every time we were able to transmit a frame over rf.
    std::uint64_t gfsk_tx_frames_transmitted{};
    
    // Every time we were unable to transmit a frame over rf.
    std::uint64_t gfsk_tx_failed_transmissions{};
    
    // Every time a packet is dropped due to failure to enable a channel.
    std::uint64_t gfsk_tx_dropped_packets{};
    
    // The total number of idle frames transmitted.
    std::uint64_t gfsk_tx_idle_frames_transmitted{};
    
    // The amount of times we tried to transmit an idle frame and it failed
    std::uint64_t gfsk_tx_failed_idle_frames_transmitted{};
    
    // The amount of times the check for bytes_in_flight failed.
    std::uint64_t gfsk_tx_failed_bytes_in_flight_checks{};
    
    // Reports if an LDO is able to supply power for a rail.
    bool ad9122_pgood{};
    
    // Reports if an LDO is able to supply power for a rail.
    bool ad9361_pgood{};
    
    // Reports if an LDO is able to supply power for a rail.
    bool adrf6780_pgood{};
    
    // Reports if an LDO is able to supply power for a rail.
    bool at86_pgood{};
    
    // Reports if this pll is locked.
    bool at86_is_pll_locked{};
    
    // The measured current going through the rail.
    double aux_3v8_isense{};
    
    // The measured voltage of the rail.
    double aux_3v8_vsense{};
    
    // The measured current going through the rail.
    double carrier_28v0_isense{};
    
    // The measured voltage of the rail.
    double carrier_28v0_vsense{};
    
    // The measured current going through the rail.
    double carrier_2v1_isense{};
    
    // The measured voltage of the rail.
    double carrier_2v1_vsense{};
    
    // The measured current going through the rail.
    double carrier_2v6_isense{};
    
    // The measured voltage of the rail.
    double carrier_2v6_vsense{};
    
    // The measured current going through the rail.
    double carrier_3v8_isense{};
    
    // The measured voltage of the rail.
    double carrier_3v8_vsense{};
    
    // The measured current going through the rail.
    double carrier_5v5_isense{};
    
    // The measured voltage of the rail.
    double carrier_5v5_vsense{};
    
    // The measured temperature for this part of the board.
    double carrier_temp{};
    
    // Reports if an LDO is able to supply power for a rail.
    bool lband_rx_pgood{};
    
    // The measured temperature for this part of the board.
    double lband_temp{};
    
    // Reports if an LDO is able to supply power for a rail.
    bool lband_tx_pgood{};
    
    // The detected power level for the rf chain.
    double lband_tx_rf_detect{};
    
    // Reports if an LDO is able to supply power for a rail.
    bool lmk04832_pgood{};
    
    // Reports if this pll is locked.
    bool lmk04832_is_pll_locked{};
    
    // Reports if an LDO is able to supply power for a rail.
    bool lmx2594_pgood{};
    
    // Reports if this pll is locked.
    bool max2771_a_1_is_pll_locked{};
    
    // Reports if this pll is locked.
    bool max2771_a_2_is_pll_locked{};
    
    // Reports if an LDO is able to supply power for a rail.
    bool max2771_a_bias_pgood{};
    
    // Reports if an LDO is able to supply power for a rail.
    bool max2771_a_pgood{};
    
    // Reports if this pll is locked.
    bool max2771_b_1_is_pll_locked{};
    
    // Reports if this pll is locked.
    bool max2771_b_2_is_pll_locked{};
    
    // Reports if an LDO is able to supply power for a rail.
    bool max2771_b_bias_pgood{};
    
    // Reports if an LDO is able to supply power for a rail.
    bool max2771_b_pgood{};
    
    // Reports if an LDO is able to supply power for a rail.
    bool rf_fe_mux_pgood{};
    
    // Reports if an LDO is able to supply power for a rail.
    bool sband_rx_pgood{};
    
    // The measured temperature for this part of the board.
    double sband_temp{};
    
    // Reports if an LDO is able to supply power for a rail.
    bool sband_tx_pgood{};
    
    // The detected power level for the rf chain.
    double sband_tx_rf_detect{};
    
    // Reports if an LDO is able to supply power for a rail.
    bool si5345_pgood{};
    
    // The measured current going through the rail.
    double som_5v0_isense{};
    
    // The measured voltage of the rail.
    double som_5v0_vsense{};
    
    // Reports if an LDO is able to supply power for a rail.
    bool uhf_rx_pgood{};
    
    // The measured temperature for this part of the board.
    double uhf_temp{};
    
    // Reports if an LDO is able to supply power for a rail.
    bool uhf_tx_pgood{};
    
    // The detected power level for the rf chain.
    double uhf_tx_rf_detect{};
    
    // The measured current going through the rail.
    double xband_24v0_isense{};
    
    // The measured voltage of the rail.
    double xband_24v0_vsense{};
    
    // Reports if an LDO is able to supply power for a rail.
    bool xband_drain_pgood{};
    
    // The measured temperature for this part of the board.
    double xband_temp{};
    
    // The detected power level for the rf chain.
    double xband_tx_rf_detect{};
    
    // placeholder
    std::uint64_t anylink_uhf_tx_sent_bytes{};
    
    // placeholder
    std::uint64_t anylink_uhf_tx_sent_packets{};
    
    // placeholder
    std::uint64_t anylink_uhf_tx_sent_frames{};
    
    // placeholder
    std::uint64_t anylink_uhf_tx_overflow_frames{};
    
    // placeholder
    std::uint64_t anylink_sband_tx_sent_bytes{};
    
    // placeholder
    std::uint64_t anylink_sband_tx_sent_packets{};
    
    // placeholder
    std::uint64_t anylink_sband_tx_sent_frames{};
    
    // placeholder
    std::uint64_t anylink_sband_tx_overflow_frames{};
    
    // placeholder
    std::uint64_t anylink_xband_tx_sent_bytes{};
    
    // placeholder
    std::uint64_t anylink_xband_tx_sent_packets{};
    
    // placeholder
    std::uint64_t anylink_xband_tx_sent_frames{};
    
    // placeholder
    std::uint64_t anylink_xband_tx_overflow_frames{};
    
    // placeholder
    std::uint64_t anylink_sband_rx_received_bytes{};
    
    // placeholder
    std::uint64_t anylink_sband_rx_received_packets{};
    
    // placeholder
    std::uint64_t anylink_sband_rx_received_frames{};
    
    // placeholder
    std::uint64_t anylink_sband_rx_dropped_packets{};
    
    // placeholder
    std::uint64_t anylink_sband_rx_dropped_frames{};
    
    // placeholder
    std::uint64_t anylink_sband_rx_socket_errors{};
    
    // placeholder
    std::uint64_t anylink_sband_rx_idle_frames{};
    
    // placeholder
    std::uint64_t anylink_heartbeats_sent{};
    
    // placeholder
    std::uint64_t anylink_heartbeats_received{};
    
    // placeholder
    std::uint64_t anylink_rx_radio_bad_header{};
    
    // placeholder
    std::uint64_t anylink_rx_radio_packets_received{};
    
    // placeholder
    std::uint64_t anylink_tx_radio_packets_send_errors{};
    
    // placeholder
    std::uint64_t anylink_tx_radio_packets_sent{};
    
    // placeholder
    std::uint64_t anylink_tx_radio_packet_nodest{};
    
    // placeholder
    std::uint64_t anylink_tx_radio_packet_truncate{};
    
    // placeholder
    std::uint64_t anylink_tx_radio_packet_pad{};
    
    // placeholder
    std::uint64_t anylink_rx_radio_no_endpoint{};
    
    // placeholder
    std::uint64_t anylink_rx_radio_reject_echo{};
    
    // placeholder
    std::uint64_t anylink_total_endpoint_packets_received{};
    
    // placeholder
    std::uint64_t anylink_total_endpoint_packets_sent{};
    
    // placeholder
    std::uint64_t anylink_encryption_failed{};
    
    // placeholder
    std::uint64_t anylink_decryption_failed{};
    
    // placeholder
    std::array<char, STRING_BUFFER_SIZE> anylink_tap_endpoint_active_tx_channel{};
    
    // placeholder
    std::uint64_t anylink_tap_endpoint_mtu{};
    
    // placeholder
    std::uint64_t anylink_tap_endpoint_recv_bytes{};
    
    // placeholder
    std::uint64_t anylink_tap_endpoint_recv_errors{};
    
    // placeholder
    std::uint64_t anylink_tap_endpoint_recv_packets{};
    
    // placeholder
    std::uint64_t anylink_tap_endpoint_send_bytes{};
    
    // placeholder
    std::uint64_t anylink_tap_endpoint_send_errors{};
    
    // placeholder
    std::uint64_t anylink_tap_endpoint_send_packets{};
    

    template <typename ObjectMap>
    void from_object_map(const ObjectMap &in)
    {
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("source_id"), this->source_id);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("schema_hash"), this->schema_hash);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("unix_timestamp_ns"), this->unix_timestamp_ns);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("controld_version"), this->controld_version);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("controld_timestamp"), this->controld_timestamp);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("powerd_version"), this->powerd_version);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("powerd_timestamp"), this->powerd_timestamp);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("radiod_version"), this->radiod_version);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("radiod_timestamp"), this->radiod_timestamp);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("fpga_version"), this->fpga_version);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("fpga_timestamp"), this->fpga_timestamp);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("fpga_project_name"), this->fpga_project_name);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_version"), this->anylink_version);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_tx_bytes_total"), this->psk_cc_tx_bytes_total);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_tx_underflows"), this->psk_cc_tx_underflows);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_tx_client_recv_errors"), this->psk_cc_tx_client_recv_errors);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_tx_client_msgs"), this->psk_cc_tx_client_msgs);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_tx_frames_transmitted"), this->psk_cc_tx_frames_transmitted);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_tx_failed_transmissions"), this->psk_cc_tx_failed_transmissions);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_tx_dropped_packets"), this->psk_cc_tx_dropped_packets);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_tx_idle_frames_transmitted"), this->psk_cc_tx_idle_frames_transmitted);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_tx_failed_idle_frames_transmitted"), this->psk_cc_tx_failed_idle_frames_transmitted);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_tx_failed_bytes_in_flight_checks"), this->psk_cc_tx_failed_bytes_in_flight_checks);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_tx_modem_underflows"), this->psk_cc_tx_modem_underflows);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_tx_ad9361_tx_pll_lock"), this->psk_cc_tx_ad9361_tx_pll_lock);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_rx_bytes_total"), this->psk_cc_rx_bytes_total);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_rx_client_send_errors"), this->psk_cc_rx_client_send_errors);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_rx_client_msgs"), this->psk_cc_rx_client_msgs);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_rx_frames_received"), this->psk_cc_rx_frames_received);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_rx_failed_receptions"), this->psk_cc_rx_failed_receptions);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_rx_dropped_good_packets"), this->psk_cc_rx_dropped_good_packets);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_rx_failed_frames_available_checks"), this->psk_cc_rx_failed_frames_available_checks);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_rx_encountered_frames_in_progress"), this->psk_cc_rx_encountered_frames_in_progress);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_rx_modem_dma_overflows"), this->psk_cc_rx_modem_dma_overflows);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_rx_modem_dma_packet_count"), this->psk_cc_rx_modem_dma_packet_count);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_rx_signal_present"), this->psk_cc_rx_signal_present);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_rx_carrier_lock"), this->psk_cc_rx_carrier_lock);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_rx_frame_sync_lock"), this->psk_cc_rx_frame_sync_lock);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_rx_fec_confirmed_lock"), this->psk_cc_rx_fec_confirmed_lock);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_rx_fec_ber"), this->psk_cc_rx_fec_ber);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_rx_ad9361_rx_pll_lock"), this->psk_cc_rx_ad9361_rx_pll_lock);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("psk_cc_rx_ad9361_bb_pll_lock"), this->psk_cc_rx_ad9361_bb_pll_lock);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("dvbs2_tx_bytes_total"), this->dvbs2_tx_bytes_total);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("dvbs2_tx_underflows"), this->dvbs2_tx_underflows);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("dvbs2_tx_client_recv_errors"), this->dvbs2_tx_client_recv_errors);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("dvbs2_tx_client_msgs"), this->dvbs2_tx_client_msgs);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("dvbs2_tx_frames_transmitted"), this->dvbs2_tx_frames_transmitted);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("dvbs2_tx_failed_transmissions"), this->dvbs2_tx_failed_transmissions);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("dvbs2_tx_dropped_packets"), this->dvbs2_tx_dropped_packets);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("dvbs2_tx_idle_frames_transmitted"), this->dvbs2_tx_idle_frames_transmitted);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("dvbs2_tx_failed_idle_frames_transmitted"), this->dvbs2_tx_failed_idle_frames_transmitted);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("dvbs2_tx_failed_bytes_in_flight_checks"), this->dvbs2_tx_failed_bytes_in_flight_checks);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("dvbs2_tx_dummy_pl_frames"), this->dvbs2_tx_dummy_pl_frames);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("gfsk_tx_bytes_total"), this->gfsk_tx_bytes_total);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("gfsk_tx_underflows"), this->gfsk_tx_underflows);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("gfsk_tx_client_recv_errors"), this->gfsk_tx_client_recv_errors);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("gfsk_tx_client_msgs"), this->gfsk_tx_client_msgs);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("gfsk_tx_frames_transmitted"), this->gfsk_tx_frames_transmitted);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("gfsk_tx_failed_transmissions"), this->gfsk_tx_failed_transmissions);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("gfsk_tx_dropped_packets"), this->gfsk_tx_dropped_packets);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("gfsk_tx_idle_frames_transmitted"), this->gfsk_tx_idle_frames_transmitted);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("gfsk_tx_failed_idle_frames_transmitted"), this->gfsk_tx_failed_idle_frames_transmitted);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("gfsk_tx_failed_bytes_in_flight_checks"), this->gfsk_tx_failed_bytes_in_flight_checks);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("ad9122_pgood"), this->ad9122_pgood);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("ad9361_pgood"), this->ad9361_pgood);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("adrf6780_pgood"), this->adrf6780_pgood);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("at86_pgood"), this->at86_pgood);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("at86_is_pll_locked"), this->at86_is_pll_locked);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("aux_3v8_isense"), this->aux_3v8_isense);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("aux_3v8_vsense"), this->aux_3v8_vsense);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("carrier_28v0_isense"), this->carrier_28v0_isense);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("carrier_28v0_vsense"), this->carrier_28v0_vsense);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("carrier_2v1_isense"), this->carrier_2v1_isense);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("carrier_2v1_vsense"), this->carrier_2v1_vsense);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("carrier_2v6_isense"), this->carrier_2v6_isense);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("carrier_2v6_vsense"), this->carrier_2v6_vsense);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("carrier_3v8_isense"), this->carrier_3v8_isense);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("carrier_3v8_vsense"), this->carrier_3v8_vsense);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("carrier_5v5_isense"), this->carrier_5v5_isense);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("carrier_5v5_vsense"), this->carrier_5v5_vsense);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("carrier_temp"), this->carrier_temp);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("lband_rx_pgood"), this->lband_rx_pgood);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("lband_temp"), this->lband_temp);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("lband_tx_pgood"), this->lband_tx_pgood);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("lband_tx_rf_detect"), this->lband_tx_rf_detect);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("lmk04832_pgood"), this->lmk04832_pgood);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("lmk04832_is_pll_locked"), this->lmk04832_is_pll_locked);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("lmx2594_pgood"), this->lmx2594_pgood);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("max2771_a_1_is_pll_locked"), this->max2771_a_1_is_pll_locked);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("max2771_a_2_is_pll_locked"), this->max2771_a_2_is_pll_locked);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("max2771_a_bias_pgood"), this->max2771_a_bias_pgood);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("max2771_a_pgood"), this->max2771_a_pgood);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("max2771_b_1_is_pll_locked"), this->max2771_b_1_is_pll_locked);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("max2771_b_2_is_pll_locked"), this->max2771_b_2_is_pll_locked);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("max2771_b_bias_pgood"), this->max2771_b_bias_pgood);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("max2771_b_pgood"), this->max2771_b_pgood);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("rf_fe_mux_pgood"), this->rf_fe_mux_pgood);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("sband_rx_pgood"), this->sband_rx_pgood);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("sband_temp"), this->sband_temp);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("sband_tx_pgood"), this->sband_tx_pgood);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("sband_tx_rf_detect"), this->sband_tx_rf_detect);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("si5345_pgood"), this->si5345_pgood);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("som_5v0_isense"), this->som_5v0_isense);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("som_5v0_vsense"), this->som_5v0_vsense);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("uhf_rx_pgood"), this->uhf_rx_pgood);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("uhf_temp"), this->uhf_temp);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("uhf_tx_pgood"), this->uhf_tx_pgood);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("uhf_tx_rf_detect"), this->uhf_tx_rf_detect);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("xband_24v0_isense"), this->xband_24v0_isense);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("xband_24v0_vsense"), this->xband_24v0_vsense);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("xband_drain_pgood"), this->xband_drain_pgood);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("xband_temp"), this->xband_temp);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("xband_tx_rf_detect"), this->xband_tx_rf_detect);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_uhf_tx_sent_bytes"), this->anylink_uhf_tx_sent_bytes);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_uhf_tx_sent_packets"), this->anylink_uhf_tx_sent_packets);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_uhf_tx_sent_frames"), this->anylink_uhf_tx_sent_frames);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_uhf_tx_overflow_frames"), this->anylink_uhf_tx_overflow_frames);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_sband_tx_sent_bytes"), this->anylink_sband_tx_sent_bytes);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_sband_tx_sent_packets"), this->anylink_sband_tx_sent_packets);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_sband_tx_sent_frames"), this->anylink_sband_tx_sent_frames);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_sband_tx_overflow_frames"), this->anylink_sband_tx_overflow_frames);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_xband_tx_sent_bytes"), this->anylink_xband_tx_sent_bytes);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_xband_tx_sent_packets"), this->anylink_xband_tx_sent_packets);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_xband_tx_sent_frames"), this->anylink_xband_tx_sent_frames);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_xband_tx_overflow_frames"), this->anylink_xband_tx_overflow_frames);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_sband_rx_received_bytes"), this->anylink_sband_rx_received_bytes);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_sband_rx_received_packets"), this->anylink_sband_rx_received_packets);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_sband_rx_received_frames"), this->anylink_sband_rx_received_frames);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_sband_rx_dropped_packets"), this->anylink_sband_rx_dropped_packets);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_sband_rx_dropped_frames"), this->anylink_sband_rx_dropped_frames);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_sband_rx_socket_errors"), this->anylink_sband_rx_socket_errors);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_sband_rx_idle_frames"), this->anylink_sband_rx_idle_frames);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_heartbeats_sent"), this->anylink_heartbeats_sent);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_heartbeats_received"), this->anylink_heartbeats_received);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_rx_radio_bad_header"), this->anylink_rx_radio_bad_header);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_rx_radio_packets_received"), this->anylink_rx_radio_packets_received);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_tx_radio_packets_send_errors"), this->anylink_tx_radio_packets_send_errors);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_tx_radio_packets_sent"), this->anylink_tx_radio_packets_sent);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_tx_radio_packet_nodest"), this->anylink_tx_radio_packet_nodest);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_tx_radio_packet_truncate"), this->anylink_tx_radio_packet_truncate);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_tx_radio_packet_pad"), this->anylink_tx_radio_packet_pad);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_rx_radio_no_endpoint"), this->anylink_rx_radio_no_endpoint);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_rx_radio_reject_echo"), this->anylink_rx_radio_reject_echo);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_total_endpoint_packets_received"), this->anylink_total_endpoint_packets_received);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_total_endpoint_packets_sent"), this->anylink_total_endpoint_packets_sent);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_encryption_failed"), this->anylink_encryption_failed);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_decryption_failed"), this->anylink_decryption_failed);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_tap_endpoint_active_tx_channel"), this->anylink_tap_endpoint_active_tx_channel);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_tap_endpoint_mtu"), this->anylink_tap_endpoint_mtu);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_tap_endpoint_recv_bytes"), this->anylink_tap_endpoint_recv_bytes);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_tap_endpoint_recv_errors"), this->anylink_tap_endpoint_recv_errors);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_tap_endpoint_recv_packets"), this->anylink_tap_endpoint_recv_packets);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_tap_endpoint_send_bytes"), this->anylink_tap_endpoint_send_bytes);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_tap_endpoint_send_errors"), this->anylink_tap_endpoint_send_errors);
        anysignal_sharemap_from_object_map_field(in, reinterpret_cast<const char *>("anylink_tap_endpoint_send_packets"), this->anylink_tap_endpoint_send_packets);
    }

    template <typename ObjectMap>
    void to_object_map(ObjectMap &out) const
    {
        anysignal_sharemap_to_object_map_field(this->source_id, out, reinterpret_cast<const char *>("source_id"));
        anysignal_sharemap_to_object_map_field(this->schema_hash, out, reinterpret_cast<const char *>("schema_hash"));
        anysignal_sharemap_to_object_map_field(this->unix_timestamp_ns, out, reinterpret_cast<const char *>("unix_timestamp_ns"));
        anysignal_sharemap_to_object_map_field(this->controld_version, out, reinterpret_cast<const char *>("controld_version"));
        anysignal_sharemap_to_object_map_field(this->controld_timestamp, out, reinterpret_cast<const char *>("controld_timestamp"));
        anysignal_sharemap_to_object_map_field(this->powerd_version, out, reinterpret_cast<const char *>("powerd_version"));
        anysignal_sharemap_to_object_map_field(this->powerd_timestamp, out, reinterpret_cast<const char *>("powerd_timestamp"));
        anysignal_sharemap_to_object_map_field(this->radiod_version, out, reinterpret_cast<const char *>("radiod_version"));
        anysignal_sharemap_to_object_map_field(this->radiod_timestamp, out, reinterpret_cast<const char *>("radiod_timestamp"));
        anysignal_sharemap_to_object_map_field(this->fpga_version, out, reinterpret_cast<const char *>("fpga_version"));
        anysignal_sharemap_to_object_map_field(this->fpga_timestamp, out, reinterpret_cast<const char *>("fpga_timestamp"));
        anysignal_sharemap_to_object_map_field(this->fpga_project_name, out, reinterpret_cast<const char *>("fpga_project_name"));
        anysignal_sharemap_to_object_map_field(this->anylink_version, out, reinterpret_cast<const char *>("anylink_version"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_tx_bytes_total, out, reinterpret_cast<const char *>("psk_cc_tx_bytes_total"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_tx_underflows, out, reinterpret_cast<const char *>("psk_cc_tx_underflows"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_tx_client_recv_errors, out, reinterpret_cast<const char *>("psk_cc_tx_client_recv_errors"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_tx_client_msgs, out, reinterpret_cast<const char *>("psk_cc_tx_client_msgs"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_tx_frames_transmitted, out, reinterpret_cast<const char *>("psk_cc_tx_frames_transmitted"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_tx_failed_transmissions, out, reinterpret_cast<const char *>("psk_cc_tx_failed_transmissions"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_tx_dropped_packets, out, reinterpret_cast<const char *>("psk_cc_tx_dropped_packets"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_tx_idle_frames_transmitted, out, reinterpret_cast<const char *>("psk_cc_tx_idle_frames_transmitted"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_tx_failed_idle_frames_transmitted, out, reinterpret_cast<const char *>("psk_cc_tx_failed_idle_frames_transmitted"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_tx_failed_bytes_in_flight_checks, out, reinterpret_cast<const char *>("psk_cc_tx_failed_bytes_in_flight_checks"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_tx_modem_underflows, out, reinterpret_cast<const char *>("psk_cc_tx_modem_underflows"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_tx_ad9361_tx_pll_lock, out, reinterpret_cast<const char *>("psk_cc_tx_ad9361_tx_pll_lock"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_rx_bytes_total, out, reinterpret_cast<const char *>("psk_cc_rx_bytes_total"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_rx_client_send_errors, out, reinterpret_cast<const char *>("psk_cc_rx_client_send_errors"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_rx_client_msgs, out, reinterpret_cast<const char *>("psk_cc_rx_client_msgs"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_rx_frames_received, out, reinterpret_cast<const char *>("psk_cc_rx_frames_received"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_rx_failed_receptions, out, reinterpret_cast<const char *>("psk_cc_rx_failed_receptions"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_rx_dropped_good_packets, out, reinterpret_cast<const char *>("psk_cc_rx_dropped_good_packets"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_rx_failed_frames_available_checks, out, reinterpret_cast<const char *>("psk_cc_rx_failed_frames_available_checks"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_rx_encountered_frames_in_progress, out, reinterpret_cast<const char *>("psk_cc_rx_encountered_frames_in_progress"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_rx_modem_dma_overflows, out, reinterpret_cast<const char *>("psk_cc_rx_modem_dma_overflows"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_rx_modem_dma_packet_count, out, reinterpret_cast<const char *>("psk_cc_rx_modem_dma_packet_count"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_rx_signal_present, out, reinterpret_cast<const char *>("psk_cc_rx_signal_present"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_rx_carrier_lock, out, reinterpret_cast<const char *>("psk_cc_rx_carrier_lock"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_rx_frame_sync_lock, out, reinterpret_cast<const char *>("psk_cc_rx_frame_sync_lock"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_rx_fec_confirmed_lock, out, reinterpret_cast<const char *>("psk_cc_rx_fec_confirmed_lock"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_rx_fec_ber, out, reinterpret_cast<const char *>("psk_cc_rx_fec_ber"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_rx_ad9361_rx_pll_lock, out, reinterpret_cast<const char *>("psk_cc_rx_ad9361_rx_pll_lock"));
        anysignal_sharemap_to_object_map_field(this->psk_cc_rx_ad9361_bb_pll_lock, out, reinterpret_cast<const char *>("psk_cc_rx_ad9361_bb_pll_lock"));
        anysignal_sharemap_to_object_map_field(this->dvbs2_tx_bytes_total, out, reinterpret_cast<const char *>("dvbs2_tx_bytes_total"));
        anysignal_sharemap_to_object_map_field(this->dvbs2_tx_underflows, out, reinterpret_cast<const char *>("dvbs2_tx_underflows"));
        anysignal_sharemap_to_object_map_field(this->dvbs2_tx_client_recv_errors, out, reinterpret_cast<const char *>("dvbs2_tx_client_recv_errors"));
        anysignal_sharemap_to_object_map_field(this->dvbs2_tx_client_msgs, out, reinterpret_cast<const char *>("dvbs2_tx_client_msgs"));
        anysignal_sharemap_to_object_map_field(this->dvbs2_tx_frames_transmitted, out, reinterpret_cast<const char *>("dvbs2_tx_frames_transmitted"));
        anysignal_sharemap_to_object_map_field(this->dvbs2_tx_failed_transmissions, out, reinterpret_cast<const char *>("dvbs2_tx_failed_transmissions"));
        anysignal_sharemap_to_object_map_field(this->dvbs2_tx_dropped_packets, out, reinterpret_cast<const char *>("dvbs2_tx_dropped_packets"));
        anysignal_sharemap_to_object_map_field(this->dvbs2_tx_idle_frames_transmitted, out, reinterpret_cast<const char *>("dvbs2_tx_idle_frames_transmitted"));
        anysignal_sharemap_to_object_map_field(this->dvbs2_tx_failed_idle_frames_transmitted, out, reinterpret_cast<const char *>("dvbs2_tx_failed_idle_frames_transmitted"));
        anysignal_sharemap_to_object_map_field(this->dvbs2_tx_failed_bytes_in_flight_checks, out, reinterpret_cast<const char *>("dvbs2_tx_failed_bytes_in_flight_checks"));
        anysignal_sharemap_to_object_map_field(this->dvbs2_tx_dummy_pl_frames, out, reinterpret_cast<const char *>("dvbs2_tx_dummy_pl_frames"));
        anysignal_sharemap_to_object_map_field(this->gfsk_tx_bytes_total, out, reinterpret_cast<const char *>("gfsk_tx_bytes_total"));
        anysignal_sharemap_to_object_map_field(this->gfsk_tx_underflows, out, reinterpret_cast<const char *>("gfsk_tx_underflows"));
        anysignal_sharemap_to_object_map_field(this->gfsk_tx_client_recv_errors, out, reinterpret_cast<const char *>("gfsk_tx_client_recv_errors"));
        anysignal_sharemap_to_object_map_field(this->gfsk_tx_client_msgs, out, reinterpret_cast<const char *>("gfsk_tx_client_msgs"));
        anysignal_sharemap_to_object_map_field(this->gfsk_tx_frames_transmitted, out, reinterpret_cast<const char *>("gfsk_tx_frames_transmitted"));
        anysignal_sharemap_to_object_map_field(this->gfsk_tx_failed_transmissions, out, reinterpret_cast<const char *>("gfsk_tx_failed_transmissions"));
        anysignal_sharemap_to_object_map_field(this->gfsk_tx_dropped_packets, out, reinterpret_cast<const char *>("gfsk_tx_dropped_packets"));
        anysignal_sharemap_to_object_map_field(this->gfsk_tx_idle_frames_transmitted, out, reinterpret_cast<const char *>("gfsk_tx_idle_frames_transmitted"));
        anysignal_sharemap_to_object_map_field(this->gfsk_tx_failed_idle_frames_transmitted, out, reinterpret_cast<const char *>("gfsk_tx_failed_idle_frames_transmitted"));
        anysignal_sharemap_to_object_map_field(this->gfsk_tx_failed_bytes_in_flight_checks, out, reinterpret_cast<const char *>("gfsk_tx_failed_bytes_in_flight_checks"));
        anysignal_sharemap_to_object_map_field(this->ad9122_pgood, out, reinterpret_cast<const char *>("ad9122_pgood"));
        anysignal_sharemap_to_object_map_field(this->ad9361_pgood, out, reinterpret_cast<const char *>("ad9361_pgood"));
        anysignal_sharemap_to_object_map_field(this->adrf6780_pgood, out, reinterpret_cast<const char *>("adrf6780_pgood"));
        anysignal_sharemap_to_object_map_field(this->at86_pgood, out, reinterpret_cast<const char *>("at86_pgood"));
        anysignal_sharemap_to_object_map_field(this->at86_is_pll_locked, out, reinterpret_cast<const char *>("at86_is_pll_locked"));
        anysignal_sharemap_to_object_map_field(this->aux_3v8_isense, out, reinterpret_cast<const char *>("aux_3v8_isense"));
        anysignal_sharemap_to_object_map_field(this->aux_3v8_vsense, out, reinterpret_cast<const char *>("aux_3v8_vsense"));
        anysignal_sharemap_to_object_map_field(this->carrier_28v0_isense, out, reinterpret_cast<const char *>("carrier_28v0_isense"));
        anysignal_sharemap_to_object_map_field(this->carrier_28v0_vsense, out, reinterpret_cast<const char *>("carrier_28v0_vsense"));
        anysignal_sharemap_to_object_map_field(this->carrier_2v1_isense, out, reinterpret_cast<const char *>("carrier_2v1_isense"));
        anysignal_sharemap_to_object_map_field(this->carrier_2v1_vsense, out, reinterpret_cast<const char *>("carrier_2v1_vsense"));
        anysignal_sharemap_to_object_map_field(this->carrier_2v6_isense, out, reinterpret_cast<const char *>("carrier_2v6_isense"));
        anysignal_sharemap_to_object_map_field(this->carrier_2v6_vsense, out, reinterpret_cast<const char *>("carrier_2v6_vsense"));
        anysignal_sharemap_to_object_map_field(this->carrier_3v8_isense, out, reinterpret_cast<const char *>("carrier_3v8_isense"));
        anysignal_sharemap_to_object_map_field(this->carrier_3v8_vsense, out, reinterpret_cast<const char *>("carrier_3v8_vsense"));
        anysignal_sharemap_to_object_map_field(this->carrier_5v5_isense, out, reinterpret_cast<const char *>("carrier_5v5_isense"));
        anysignal_sharemap_to_object_map_field(this->carrier_5v5_vsense, out, reinterpret_cast<const char *>("carrier_5v5_vsense"));
        anysignal_sharemap_to_object_map_field(this->carrier_temp, out, reinterpret_cast<const char *>("carrier_temp"));
        anysignal_sharemap_to_object_map_field(this->lband_rx_pgood, out, reinterpret_cast<const char *>("lband_rx_pgood"));
        anysignal_sharemap_to_object_map_field(this->lband_temp, out, reinterpret_cast<const char *>("lband_temp"));
        anysignal_sharemap_to_object_map_field(this->lband_tx_pgood, out, reinterpret_cast<const char *>("lband_tx_pgood"));
        anysignal_sharemap_to_object_map_field(this->lband_tx_rf_detect, out, reinterpret_cast<const char *>("lband_tx_rf_detect"));
        anysignal_sharemap_to_object_map_field(this->lmk04832_pgood, out, reinterpret_cast<const char *>("lmk04832_pgood"));
        anysignal_sharemap_to_object_map_field(this->lmk04832_is_pll_locked, out, reinterpret_cast<const char *>("lmk04832_is_pll_locked"));
        anysignal_sharemap_to_object_map_field(this->lmx2594_pgood, out, reinterpret_cast<const char *>("lmx2594_pgood"));
        anysignal_sharemap_to_object_map_field(this->max2771_a_1_is_pll_locked, out, reinterpret_cast<const char *>("max2771_a_1_is_pll_locked"));
        anysignal_sharemap_to_object_map_field(this->max2771_a_2_is_pll_locked, out, reinterpret_cast<const char *>("max2771_a_2_is_pll_locked"));
        anysignal_sharemap_to_object_map_field(this->max2771_a_bias_pgood, out, reinterpret_cast<const char *>("max2771_a_bias_pgood"));
        anysignal_sharemap_to_object_map_field(this->max2771_a_pgood, out, reinterpret_cast<const char *>("max2771_a_pgood"));
        anysignal_sharemap_to_object_map_field(this->max2771_b_1_is_pll_locked, out, reinterpret_cast<const char *>("max2771_b_1_is_pll_locked"));
        anysignal_sharemap_to_object_map_field(this->max2771_b_2_is_pll_locked, out, reinterpret_cast<const char *>("max2771_b_2_is_pll_locked"));
        anysignal_sharemap_to_object_map_field(this->max2771_b_bias_pgood, out, reinterpret_cast<const char *>("max2771_b_bias_pgood"));
        anysignal_sharemap_to_object_map_field(this->max2771_b_pgood, out, reinterpret_cast<const char *>("max2771_b_pgood"));
        anysignal_sharemap_to_object_map_field(this->rf_fe_mux_pgood, out, reinterpret_cast<const char *>("rf_fe_mux_pgood"));
        anysignal_sharemap_to_object_map_field(this->sband_rx_pgood, out, reinterpret_cast<const char *>("sband_rx_pgood"));
        anysignal_sharemap_to_object_map_field(this->sband_temp, out, reinterpret_cast<const char *>("sband_temp"));
        anysignal_sharemap_to_object_map_field(this->sband_tx_pgood, out, reinterpret_cast<const char *>("sband_tx_pgood"));
        anysignal_sharemap_to_object_map_field(this->sband_tx_rf_detect, out, reinterpret_cast<const char *>("sband_tx_rf_detect"));
        anysignal_sharemap_to_object_map_field(this->si5345_pgood, out, reinterpret_cast<const char *>("si5345_pgood"));
        anysignal_sharemap_to_object_map_field(this->som_5v0_isense, out, reinterpret_cast<const char *>("som_5v0_isense"));
        anysignal_sharemap_to_object_map_field(this->som_5v0_vsense, out, reinterpret_cast<const char *>("som_5v0_vsense"));
        anysignal_sharemap_to_object_map_field(this->uhf_rx_pgood, out, reinterpret_cast<const char *>("uhf_rx_pgood"));
        anysignal_sharemap_to_object_map_field(this->uhf_temp, out, reinterpret_cast<const char *>("uhf_temp"));
        anysignal_sharemap_to_object_map_field(this->uhf_tx_pgood, out, reinterpret_cast<const char *>("uhf_tx_pgood"));
        anysignal_sharemap_to_object_map_field(this->uhf_tx_rf_detect, out, reinterpret_cast<const char *>("uhf_tx_rf_detect"));
        anysignal_sharemap_to_object_map_field(this->xband_24v0_isense, out, reinterpret_cast<const char *>("xband_24v0_isense"));
        anysignal_sharemap_to_object_map_field(this->xband_24v0_vsense, out, reinterpret_cast<const char *>("xband_24v0_vsense"));
        anysignal_sharemap_to_object_map_field(this->xband_drain_pgood, out, reinterpret_cast<const char *>("xband_drain_pgood"));
        anysignal_sharemap_to_object_map_field(this->xband_temp, out, reinterpret_cast<const char *>("xband_temp"));
        anysignal_sharemap_to_object_map_field(this->xband_tx_rf_detect, out, reinterpret_cast<const char *>("xband_tx_rf_detect"));
        anysignal_sharemap_to_object_map_field(this->anylink_uhf_tx_sent_bytes, out, reinterpret_cast<const char *>("anylink_uhf_tx_sent_bytes"));
        anysignal_sharemap_to_object_map_field(this->anylink_uhf_tx_sent_packets, out, reinterpret_cast<const char *>("anylink_uhf_tx_sent_packets"));
        anysignal_sharemap_to_object_map_field(this->anylink_uhf_tx_sent_frames, out, reinterpret_cast<const char *>("anylink_uhf_tx_sent_frames"));
        anysignal_sharemap_to_object_map_field(this->anylink_uhf_tx_overflow_frames, out, reinterpret_cast<const char *>("anylink_uhf_tx_overflow_frames"));
        anysignal_sharemap_to_object_map_field(this->anylink_sband_tx_sent_bytes, out, reinterpret_cast<const char *>("anylink_sband_tx_sent_bytes"));
        anysignal_sharemap_to_object_map_field(this->anylink_sband_tx_sent_packets, out, reinterpret_cast<const char *>("anylink_sband_tx_sent_packets"));
        anysignal_sharemap_to_object_map_field(this->anylink_sband_tx_sent_frames, out, reinterpret_cast<const char *>("anylink_sband_tx_sent_frames"));
        anysignal_sharemap_to_object_map_field(this->anylink_sband_tx_overflow_frames, out, reinterpret_cast<const char *>("anylink_sband_tx_overflow_frames"));
        anysignal_sharemap_to_object_map_field(this->anylink_xband_tx_sent_bytes, out, reinterpret_cast<const char *>("anylink_xband_tx_sent_bytes"));
        anysignal_sharemap_to_object_map_field(this->anylink_xband_tx_sent_packets, out, reinterpret_cast<const char *>("anylink_xband_tx_sent_packets"));
        anysignal_sharemap_to_object_map_field(this->anylink_xband_tx_sent_frames, out, reinterpret_cast<const char *>("anylink_xband_tx_sent_frames"));
        anysignal_sharemap_to_object_map_field(this->anylink_xband_tx_overflow_frames, out, reinterpret_cast<const char *>("anylink_xband_tx_overflow_frames"));
        anysignal_sharemap_to_object_map_field(this->anylink_sband_rx_received_bytes, out, reinterpret_cast<const char *>("anylink_sband_rx_received_bytes"));
        anysignal_sharemap_to_object_map_field(this->anylink_sband_rx_received_packets, out, reinterpret_cast<const char *>("anylink_sband_rx_received_packets"));
        anysignal_sharemap_to_object_map_field(this->anylink_sband_rx_received_frames, out, reinterpret_cast<const char *>("anylink_sband_rx_received_frames"));
        anysignal_sharemap_to_object_map_field(this->anylink_sband_rx_dropped_packets, out, reinterpret_cast<const char *>("anylink_sband_rx_dropped_packets"));
        anysignal_sharemap_to_object_map_field(this->anylink_sband_rx_dropped_frames, out, reinterpret_cast<const char *>("anylink_sband_rx_dropped_frames"));
        anysignal_sharemap_to_object_map_field(this->anylink_sband_rx_socket_errors, out, reinterpret_cast<const char *>("anylink_sband_rx_socket_errors"));
        anysignal_sharemap_to_object_map_field(this->anylink_sband_rx_idle_frames, out, reinterpret_cast<const char *>("anylink_sband_rx_idle_frames"));
        anysignal_sharemap_to_object_map_field(this->anylink_heartbeats_sent, out, reinterpret_cast<const char *>("anylink_heartbeats_sent"));
        anysignal_sharemap_to_object_map_field(this->anylink_heartbeats_received, out, reinterpret_cast<const char *>("anylink_heartbeats_received"));
        anysignal_sharemap_to_object_map_field(this->anylink_rx_radio_bad_header, out, reinterpret_cast<const char *>("anylink_rx_radio_bad_header"));
        anysignal_sharemap_to_object_map_field(this->anylink_rx_radio_packets_received, out, reinterpret_cast<const char *>("anylink_rx_radio_packets_received"));
        anysignal_sharemap_to_object_map_field(this->anylink_tx_radio_packets_send_errors, out, reinterpret_cast<const char *>("anylink_tx_radio_packets_send_errors"));
        anysignal_sharemap_to_object_map_field(this->anylink_tx_radio_packets_sent, out, reinterpret_cast<const char *>("anylink_tx_radio_packets_sent"));
        anysignal_sharemap_to_object_map_field(this->anylink_tx_radio_packet_nodest, out, reinterpret_cast<const char *>("anylink_tx_radio_packet_nodest"));
        anysignal_sharemap_to_object_map_field(this->anylink_tx_radio_packet_truncate, out, reinterpret_cast<const char *>("anylink_tx_radio_packet_truncate"));
        anysignal_sharemap_to_object_map_field(this->anylink_tx_radio_packet_pad, out, reinterpret_cast<const char *>("anylink_tx_radio_packet_pad"));
        anysignal_sharemap_to_object_map_field(this->anylink_rx_radio_no_endpoint, out, reinterpret_cast<const char *>("anylink_rx_radio_no_endpoint"));
        anysignal_sharemap_to_object_map_field(this->anylink_rx_radio_reject_echo, out, reinterpret_cast<const char *>("anylink_rx_radio_reject_echo"));
        anysignal_sharemap_to_object_map_field(this->anylink_total_endpoint_packets_received, out, reinterpret_cast<const char *>("anylink_total_endpoint_packets_received"));
        anysignal_sharemap_to_object_map_field(this->anylink_total_endpoint_packets_sent, out, reinterpret_cast<const char *>("anylink_total_endpoint_packets_sent"));
        anysignal_sharemap_to_object_map_field(this->anylink_encryption_failed, out, reinterpret_cast<const char *>("anylink_encryption_failed"));
        anysignal_sharemap_to_object_map_field(this->anylink_decryption_failed, out, reinterpret_cast<const char *>("anylink_decryption_failed"));
        anysignal_sharemap_to_object_map_field(this->anylink_tap_endpoint_active_tx_channel, out, reinterpret_cast<const char *>("anylink_tap_endpoint_active_tx_channel"));
        anysignal_sharemap_to_object_map_field(this->anylink_tap_endpoint_mtu, out, reinterpret_cast<const char *>("anylink_tap_endpoint_mtu"));
        anysignal_sharemap_to_object_map_field(this->anylink_tap_endpoint_recv_bytes, out, reinterpret_cast<const char *>("anylink_tap_endpoint_recv_bytes"));
        anysignal_sharemap_to_object_map_field(this->anylink_tap_endpoint_recv_errors, out, reinterpret_cast<const char *>("anylink_tap_endpoint_recv_errors"));
        anysignal_sharemap_to_object_map_field(this->anylink_tap_endpoint_recv_packets, out, reinterpret_cast<const char *>("anylink_tap_endpoint_recv_packets"));
        anysignal_sharemap_to_object_map_field(this->anylink_tap_endpoint_send_bytes, out, reinterpret_cast<const char *>("anylink_tap_endpoint_send_bytes"));
        anysignal_sharemap_to_object_map_field(this->anylink_tap_endpoint_send_errors, out, reinterpret_cast<const char *>("anylink_tap_endpoint_send_errors"));
        anysignal_sharemap_to_object_map_field(this->anylink_tap_endpoint_send_packets, out, reinterpret_cast<const char *>("anylink_tap_endpoint_send_packets"));
    }
};

static inline sharemap_metrics_packed_t sharemap_pack(sharemap_metrics_t &in)
{
    in.unix_timestamp_ns = time_ns_since_epoch();
    sharemap_metrics_packed_t out{};
    anysignal_sharemap_pack_field(in, out, source_id);
    anysignal_sharemap_pack_field(in, out, schema_hash);
    anysignal_sharemap_pack_field(in, out, unix_timestamp_ns);
    anysignal_sharemap_pack_field(in, out, controld_version);
    anysignal_sharemap_pack_field(in, out, controld_timestamp);
    anysignal_sharemap_pack_field(in, out, powerd_version);
    anysignal_sharemap_pack_field(in, out, powerd_timestamp);
    anysignal_sharemap_pack_field(in, out, radiod_version);
    anysignal_sharemap_pack_field(in, out, radiod_timestamp);
    anysignal_sharemap_pack_field(in, out, fpga_version);
    anysignal_sharemap_pack_field(in, out, fpga_timestamp);
    anysignal_sharemap_pack_field(in, out, fpga_project_name);
    anysignal_sharemap_pack_field(in, out, anylink_version);
    anysignal_sharemap_pack_field(in, out, psk_cc_tx_bytes_total);
    anysignal_sharemap_pack_field(in, out, psk_cc_tx_underflows);
    anysignal_sharemap_pack_field(in, out, psk_cc_tx_client_recv_errors);
    anysignal_sharemap_pack_field(in, out, psk_cc_tx_client_msgs);
    anysignal_sharemap_pack_field(in, out, psk_cc_tx_frames_transmitted);
    anysignal_sharemap_pack_field(in, out, psk_cc_tx_failed_transmissions);
    anysignal_sharemap_pack_field(in, out, psk_cc_tx_dropped_packets);
    anysignal_sharemap_pack_field(in, out, psk_cc_tx_idle_frames_transmitted);
    anysignal_sharemap_pack_field(in, out, psk_cc_tx_failed_idle_frames_transmitted);
    anysignal_sharemap_pack_field(in, out, psk_cc_tx_failed_bytes_in_flight_checks);
    anysignal_sharemap_pack_field(in, out, psk_cc_tx_modem_underflows);
    anysignal_sharemap_pack_field(in, out, psk_cc_tx_ad9361_tx_pll_lock);
    anysignal_sharemap_pack_field(in, out, psk_cc_rx_bytes_total);
    anysignal_sharemap_pack_field(in, out, psk_cc_rx_client_send_errors);
    anysignal_sharemap_pack_field(in, out, psk_cc_rx_client_msgs);
    anysignal_sharemap_pack_field(in, out, psk_cc_rx_frames_received);
    anysignal_sharemap_pack_field(in, out, psk_cc_rx_failed_receptions);
    anysignal_sharemap_pack_field(in, out, psk_cc_rx_dropped_good_packets);
    anysignal_sharemap_pack_field(in, out, psk_cc_rx_failed_frames_available_checks);
    anysignal_sharemap_pack_field(in, out, psk_cc_rx_encountered_frames_in_progress);
    anysignal_sharemap_pack_field(in, out, psk_cc_rx_modem_dma_overflows);
    anysignal_sharemap_pack_field(in, out, psk_cc_rx_modem_dma_packet_count);
    anysignal_sharemap_pack_field(in, out, psk_cc_rx_signal_present);
    anysignal_sharemap_pack_field(in, out, psk_cc_rx_carrier_lock);
    anysignal_sharemap_pack_field(in, out, psk_cc_rx_frame_sync_lock);
    anysignal_sharemap_pack_field(in, out, psk_cc_rx_fec_confirmed_lock);
    anysignal_sharemap_pack_field(in, out, psk_cc_rx_fec_ber);
    anysignal_sharemap_pack_field(in, out, psk_cc_rx_ad9361_rx_pll_lock);
    anysignal_sharemap_pack_field(in, out, psk_cc_rx_ad9361_bb_pll_lock);
    anysignal_sharemap_pack_field(in, out, dvbs2_tx_bytes_total);
    anysignal_sharemap_pack_field(in, out, dvbs2_tx_underflows);
    anysignal_sharemap_pack_field(in, out, dvbs2_tx_client_recv_errors);
    anysignal_sharemap_pack_field(in, out, dvbs2_tx_client_msgs);
    anysignal_sharemap_pack_field(in, out, dvbs2_tx_frames_transmitted);
    anysignal_sharemap_pack_field(in, out, dvbs2_tx_failed_transmissions);
    anysignal_sharemap_pack_field(in, out, dvbs2_tx_dropped_packets);
    anysignal_sharemap_pack_field(in, out, dvbs2_tx_idle_frames_transmitted);
    anysignal_sharemap_pack_field(in, out, dvbs2_tx_failed_idle_frames_transmitted);
    anysignal_sharemap_pack_field(in, out, dvbs2_tx_failed_bytes_in_flight_checks);
    anysignal_sharemap_pack_field(in, out, dvbs2_tx_dummy_pl_frames);
    anysignal_sharemap_pack_field(in, out, gfsk_tx_bytes_total);
    anysignal_sharemap_pack_field(in, out, gfsk_tx_underflows);
    anysignal_sharemap_pack_field(in, out, gfsk_tx_client_recv_errors);
    anysignal_sharemap_pack_field(in, out, gfsk_tx_client_msgs);
    anysignal_sharemap_pack_field(in, out, gfsk_tx_frames_transmitted);
    anysignal_sharemap_pack_field(in, out, gfsk_tx_failed_transmissions);
    anysignal_sharemap_pack_field(in, out, gfsk_tx_dropped_packets);
    anysignal_sharemap_pack_field(in, out, gfsk_tx_idle_frames_transmitted);
    anysignal_sharemap_pack_field(in, out, gfsk_tx_failed_idle_frames_transmitted);
    anysignal_sharemap_pack_field(in, out, gfsk_tx_failed_bytes_in_flight_checks);
    anysignal_sharemap_pack_field(in, out, ad9122_pgood);
    anysignal_sharemap_pack_field(in, out, ad9361_pgood);
    anysignal_sharemap_pack_field(in, out, adrf6780_pgood);
    anysignal_sharemap_pack_field(in, out, at86_pgood);
    anysignal_sharemap_pack_field(in, out, at86_is_pll_locked);
    anysignal_sharemap_pack_field(in, out, aux_3v8_isense);
    anysignal_sharemap_pack_field(in, out, aux_3v8_vsense);
    anysignal_sharemap_pack_field(in, out, carrier_28v0_isense);
    anysignal_sharemap_pack_field(in, out, carrier_28v0_vsense);
    anysignal_sharemap_pack_field(in, out, carrier_2v1_isense);
    anysignal_sharemap_pack_field(in, out, carrier_2v1_vsense);
    anysignal_sharemap_pack_field(in, out, carrier_2v6_isense);
    anysignal_sharemap_pack_field(in, out, carrier_2v6_vsense);
    anysignal_sharemap_pack_field(in, out, carrier_3v8_isense);
    anysignal_sharemap_pack_field(in, out, carrier_3v8_vsense);
    anysignal_sharemap_pack_field(in, out, carrier_5v5_isense);
    anysignal_sharemap_pack_field(in, out, carrier_5v5_vsense);
    anysignal_sharemap_pack_field(in, out, carrier_temp);
    anysignal_sharemap_pack_field(in, out, lband_rx_pgood);
    anysignal_sharemap_pack_field(in, out, lband_temp);
    anysignal_sharemap_pack_field(in, out, lband_tx_pgood);
    anysignal_sharemap_pack_field(in, out, lband_tx_rf_detect);
    anysignal_sharemap_pack_field(in, out, lmk04832_pgood);
    anysignal_sharemap_pack_field(in, out, lmk04832_is_pll_locked);
    anysignal_sharemap_pack_field(in, out, lmx2594_pgood);
    anysignal_sharemap_pack_field(in, out, max2771_a_1_is_pll_locked);
    anysignal_sharemap_pack_field(in, out, max2771_a_2_is_pll_locked);
    anysignal_sharemap_pack_field(in, out, max2771_a_bias_pgood);
    anysignal_sharemap_pack_field(in, out, max2771_a_pgood);
    anysignal_sharemap_pack_field(in, out, max2771_b_1_is_pll_locked);
    anysignal_sharemap_pack_field(in, out, max2771_b_2_is_pll_locked);
    anysignal_sharemap_pack_field(in, out, max2771_b_bias_pgood);
    anysignal_sharemap_pack_field(in, out, max2771_b_pgood);
    anysignal_sharemap_pack_field(in, out, rf_fe_mux_pgood);
    anysignal_sharemap_pack_field(in, out, sband_rx_pgood);
    anysignal_sharemap_pack_field(in, out, sband_temp);
    anysignal_sharemap_pack_field(in, out, sband_tx_pgood);
    anysignal_sharemap_pack_field(in, out, sband_tx_rf_detect);
    anysignal_sharemap_pack_field(in, out, si5345_pgood);
    anysignal_sharemap_pack_field(in, out, som_5v0_isense);
    anysignal_sharemap_pack_field(in, out, som_5v0_vsense);
    anysignal_sharemap_pack_field(in, out, uhf_rx_pgood);
    anysignal_sharemap_pack_field(in, out, uhf_temp);
    anysignal_sharemap_pack_field(in, out, uhf_tx_pgood);
    anysignal_sharemap_pack_field(in, out, uhf_tx_rf_detect);
    anysignal_sharemap_pack_field(in, out, xband_24v0_isense);
    anysignal_sharemap_pack_field(in, out, xband_24v0_vsense);
    anysignal_sharemap_pack_field(in, out, xband_drain_pgood);
    anysignal_sharemap_pack_field(in, out, xband_temp);
    anysignal_sharemap_pack_field(in, out, xband_tx_rf_detect);
    anysignal_sharemap_pack_field(in, out, anylink_uhf_tx_sent_bytes);
    anysignal_sharemap_pack_field(in, out, anylink_uhf_tx_sent_packets);
    anysignal_sharemap_pack_field(in, out, anylink_uhf_tx_sent_frames);
    anysignal_sharemap_pack_field(in, out, anylink_uhf_tx_overflow_frames);
    anysignal_sharemap_pack_field(in, out, anylink_sband_tx_sent_bytes);
    anysignal_sharemap_pack_field(in, out, anylink_sband_tx_sent_packets);
    anysignal_sharemap_pack_field(in, out, anylink_sband_tx_sent_frames);
    anysignal_sharemap_pack_field(in, out, anylink_sband_tx_overflow_frames);
    anysignal_sharemap_pack_field(in, out, anylink_xband_tx_sent_bytes);
    anysignal_sharemap_pack_field(in, out, anylink_xband_tx_sent_packets);
    anysignal_sharemap_pack_field(in, out, anylink_xband_tx_sent_frames);
    anysignal_sharemap_pack_field(in, out, anylink_xband_tx_overflow_frames);
    anysignal_sharemap_pack_field(in, out, anylink_sband_rx_received_bytes);
    anysignal_sharemap_pack_field(in, out, anylink_sband_rx_received_packets);
    anysignal_sharemap_pack_field(in, out, anylink_sband_rx_received_frames);
    anysignal_sharemap_pack_field(in, out, anylink_sband_rx_dropped_packets);
    anysignal_sharemap_pack_field(in, out, anylink_sband_rx_dropped_frames);
    anysignal_sharemap_pack_field(in, out, anylink_sband_rx_socket_errors);
    anysignal_sharemap_pack_field(in, out, anylink_sband_rx_idle_frames);
    anysignal_sharemap_pack_field(in, out, anylink_heartbeats_sent);
    anysignal_sharemap_pack_field(in, out, anylink_heartbeats_received);
    anysignal_sharemap_pack_field(in, out, anylink_rx_radio_bad_header);
    anysignal_sharemap_pack_field(in, out, anylink_rx_radio_packets_received);
    anysignal_sharemap_pack_field(in, out, anylink_tx_radio_packets_send_errors);
    anysignal_sharemap_pack_field(in, out, anylink_tx_radio_packets_sent);
    anysignal_sharemap_pack_field(in, out, anylink_tx_radio_packet_nodest);
    anysignal_sharemap_pack_field(in, out, anylink_tx_radio_packet_truncate);
    anysignal_sharemap_pack_field(in, out, anylink_tx_radio_packet_pad);
    anysignal_sharemap_pack_field(in, out, anylink_rx_radio_no_endpoint);
    anysignal_sharemap_pack_field(in, out, anylink_rx_radio_reject_echo);
    anysignal_sharemap_pack_field(in, out, anylink_total_endpoint_packets_received);
    anysignal_sharemap_pack_field(in, out, anylink_total_endpoint_packets_sent);
    anysignal_sharemap_pack_field(in, out, anylink_encryption_failed);
    anysignal_sharemap_pack_field(in, out, anylink_decryption_failed);
    anysignal_sharemap_pack_field(in, out, anylink_tap_endpoint_active_tx_channel);
    anysignal_sharemap_pack_field(in, out, anylink_tap_endpoint_mtu);
    anysignal_sharemap_pack_field(in, out, anylink_tap_endpoint_recv_bytes);
    anysignal_sharemap_pack_field(in, out, anylink_tap_endpoint_recv_errors);
    anysignal_sharemap_pack_field(in, out, anylink_tap_endpoint_recv_packets);
    anysignal_sharemap_pack_field(in, out, anylink_tap_endpoint_send_bytes);
    anysignal_sharemap_pack_field(in, out, anylink_tap_endpoint_send_errors);
    anysignal_sharemap_pack_field(in, out, anylink_tap_endpoint_send_packets);
    return out;
}

static inline sharemap_metrics_t sharemap_unpack(const sharemap_metrics_packed_t &in)
{
    sharemap_metrics_t out{};
    anysignal_sharemap_unpack_field(in, out, source_id);
    anysignal_sharemap_unpack_field(in, out, schema_hash);
    anysignal_sharemap_unpack_field(in, out, unix_timestamp_ns);
    anysignal_sharemap_unpack_field(in, out, controld_version);
    anysignal_sharemap_unpack_field(in, out, controld_timestamp);
    anysignal_sharemap_unpack_field(in, out, powerd_version);
    anysignal_sharemap_unpack_field(in, out, powerd_timestamp);
    anysignal_sharemap_unpack_field(in, out, radiod_version);
    anysignal_sharemap_unpack_field(in, out, radiod_timestamp);
    anysignal_sharemap_unpack_field(in, out, fpga_version);
    anysignal_sharemap_unpack_field(in, out, fpga_timestamp);
    anysignal_sharemap_unpack_field(in, out, fpga_project_name);
    anysignal_sharemap_unpack_field(in, out, anylink_version);
    anysignal_sharemap_unpack_field(in, out, psk_cc_tx_bytes_total);
    anysignal_sharemap_unpack_field(in, out, psk_cc_tx_underflows);
    anysignal_sharemap_unpack_field(in, out, psk_cc_tx_client_recv_errors);
    anysignal_sharemap_unpack_field(in, out, psk_cc_tx_client_msgs);
    anysignal_sharemap_unpack_field(in, out, psk_cc_tx_frames_transmitted);
    anysignal_sharemap_unpack_field(in, out, psk_cc_tx_failed_transmissions);
    anysignal_sharemap_unpack_field(in, out, psk_cc_tx_dropped_packets);
    anysignal_sharemap_unpack_field(in, out, psk_cc_tx_idle_frames_transmitted);
    anysignal_sharemap_unpack_field(in, out, psk_cc_tx_failed_idle_frames_transmitted);
    anysignal_sharemap_unpack_field(in, out, psk_cc_tx_failed_bytes_in_flight_checks);
    anysignal_sharemap_unpack_field(in, out, psk_cc_tx_modem_underflows);
    anysignal_sharemap_unpack_field(in, out, psk_cc_tx_ad9361_tx_pll_lock);
    anysignal_sharemap_unpack_field(in, out, psk_cc_rx_bytes_total);
    anysignal_sharemap_unpack_field(in, out, psk_cc_rx_client_send_errors);
    anysignal_sharemap_unpack_field(in, out, psk_cc_rx_client_msgs);
    anysignal_sharemap_unpack_field(in, out, psk_cc_rx_frames_received);
    anysignal_sharemap_unpack_field(in, out, psk_cc_rx_failed_receptions);
    anysignal_sharemap_unpack_field(in, out, psk_cc_rx_dropped_good_packets);
    anysignal_sharemap_unpack_field(in, out, psk_cc_rx_failed_frames_available_checks);
    anysignal_sharemap_unpack_field(in, out, psk_cc_rx_encountered_frames_in_progress);
    anysignal_sharemap_unpack_field(in, out, psk_cc_rx_modem_dma_overflows);
    anysignal_sharemap_unpack_field(in, out, psk_cc_rx_modem_dma_packet_count);
    anysignal_sharemap_unpack_field(in, out, psk_cc_rx_signal_present);
    anysignal_sharemap_unpack_field(in, out, psk_cc_rx_carrier_lock);
    anysignal_sharemap_unpack_field(in, out, psk_cc_rx_frame_sync_lock);
    anysignal_sharemap_unpack_field(in, out, psk_cc_rx_fec_confirmed_lock);
    anysignal_sharemap_unpack_field(in, out, psk_cc_rx_fec_ber);
    anysignal_sharemap_unpack_field(in, out, psk_cc_rx_ad9361_rx_pll_lock);
    anysignal_sharemap_unpack_field(in, out, psk_cc_rx_ad9361_bb_pll_lock);
    anysignal_sharemap_unpack_field(in, out, dvbs2_tx_bytes_total);
    anysignal_sharemap_unpack_field(in, out, dvbs2_tx_underflows);
    anysignal_sharemap_unpack_field(in, out, dvbs2_tx_client_recv_errors);
    anysignal_sharemap_unpack_field(in, out, dvbs2_tx_client_msgs);
    anysignal_sharemap_unpack_field(in, out, dvbs2_tx_frames_transmitted);
    anysignal_sharemap_unpack_field(in, out, dvbs2_tx_failed_transmissions);
    anysignal_sharemap_unpack_field(in, out, dvbs2_tx_dropped_packets);
    anysignal_sharemap_unpack_field(in, out, dvbs2_tx_idle_frames_transmitted);
    anysignal_sharemap_unpack_field(in, out, dvbs2_tx_failed_idle_frames_transmitted);
    anysignal_sharemap_unpack_field(in, out, dvbs2_tx_failed_bytes_in_flight_checks);
    anysignal_sharemap_unpack_field(in, out, dvbs2_tx_dummy_pl_frames);
    anysignal_sharemap_unpack_field(in, out, gfsk_tx_bytes_total);
    anysignal_sharemap_unpack_field(in, out, gfsk_tx_underflows);
    anysignal_sharemap_unpack_field(in, out, gfsk_tx_client_recv_errors);
    anysignal_sharemap_unpack_field(in, out, gfsk_tx_client_msgs);
    anysignal_sharemap_unpack_field(in, out, gfsk_tx_frames_transmitted);
    anysignal_sharemap_unpack_field(in, out, gfsk_tx_failed_transmissions);
    anysignal_sharemap_unpack_field(in, out, gfsk_tx_dropped_packets);
    anysignal_sharemap_unpack_field(in, out, gfsk_tx_idle_frames_transmitted);
    anysignal_sharemap_unpack_field(in, out, gfsk_tx_failed_idle_frames_transmitted);
    anysignal_sharemap_unpack_field(in, out, gfsk_tx_failed_bytes_in_flight_checks);
    anysignal_sharemap_unpack_field(in, out, ad9122_pgood);
    anysignal_sharemap_unpack_field(in, out, ad9361_pgood);
    anysignal_sharemap_unpack_field(in, out, adrf6780_pgood);
    anysignal_sharemap_unpack_field(in, out, at86_pgood);
    anysignal_sharemap_unpack_field(in, out, at86_is_pll_locked);
    anysignal_sharemap_unpack_field(in, out, aux_3v8_isense);
    anysignal_sharemap_unpack_field(in, out, aux_3v8_vsense);
    anysignal_sharemap_unpack_field(in, out, carrier_28v0_isense);
    anysignal_sharemap_unpack_field(in, out, carrier_28v0_vsense);
    anysignal_sharemap_unpack_field(in, out, carrier_2v1_isense);
    anysignal_sharemap_unpack_field(in, out, carrier_2v1_vsense);
    anysignal_sharemap_unpack_field(in, out, carrier_2v6_isense);
    anysignal_sharemap_unpack_field(in, out, carrier_2v6_vsense);
    anysignal_sharemap_unpack_field(in, out, carrier_3v8_isense);
    anysignal_sharemap_unpack_field(in, out, carrier_3v8_vsense);
    anysignal_sharemap_unpack_field(in, out, carrier_5v5_isense);
    anysignal_sharemap_unpack_field(in, out, carrier_5v5_vsense);
    anysignal_sharemap_unpack_field(in, out, carrier_temp);
    anysignal_sharemap_unpack_field(in, out, lband_rx_pgood);
    anysignal_sharemap_unpack_field(in, out, lband_temp);
    anysignal_sharemap_unpack_field(in, out, lband_tx_pgood);
    anysignal_sharemap_unpack_field(in, out, lband_tx_rf_detect);
    anysignal_sharemap_unpack_field(in, out, lmk04832_pgood);
    anysignal_sharemap_unpack_field(in, out, lmk04832_is_pll_locked);
    anysignal_sharemap_unpack_field(in, out, lmx2594_pgood);
    anysignal_sharemap_unpack_field(in, out, max2771_a_1_is_pll_locked);
    anysignal_sharemap_unpack_field(in, out, max2771_a_2_is_pll_locked);
    anysignal_sharemap_unpack_field(in, out, max2771_a_bias_pgood);
    anysignal_sharemap_unpack_field(in, out, max2771_a_pgood);
    anysignal_sharemap_unpack_field(in, out, max2771_b_1_is_pll_locked);
    anysignal_sharemap_unpack_field(in, out, max2771_b_2_is_pll_locked);
    anysignal_sharemap_unpack_field(in, out, max2771_b_bias_pgood);
    anysignal_sharemap_unpack_field(in, out, max2771_b_pgood);
    anysignal_sharemap_unpack_field(in, out, rf_fe_mux_pgood);
    anysignal_sharemap_unpack_field(in, out, sband_rx_pgood);
    anysignal_sharemap_unpack_field(in, out, sband_temp);
    anysignal_sharemap_unpack_field(in, out, sband_tx_pgood);
    anysignal_sharemap_unpack_field(in, out, sband_tx_rf_detect);
    anysignal_sharemap_unpack_field(in, out, si5345_pgood);
    anysignal_sharemap_unpack_field(in, out, som_5v0_isense);
    anysignal_sharemap_unpack_field(in, out, som_5v0_vsense);
    anysignal_sharemap_unpack_field(in, out, uhf_rx_pgood);
    anysignal_sharemap_unpack_field(in, out, uhf_temp);
    anysignal_sharemap_unpack_field(in, out, uhf_tx_pgood);
    anysignal_sharemap_unpack_field(in, out, uhf_tx_rf_detect);
    anysignal_sharemap_unpack_field(in, out, xband_24v0_isense);
    anysignal_sharemap_unpack_field(in, out, xband_24v0_vsense);
    anysignal_sharemap_unpack_field(in, out, xband_drain_pgood);
    anysignal_sharemap_unpack_field(in, out, xband_temp);
    anysignal_sharemap_unpack_field(in, out, xband_tx_rf_detect);
    anysignal_sharemap_unpack_field(in, out, anylink_uhf_tx_sent_bytes);
    anysignal_sharemap_unpack_field(in, out, anylink_uhf_tx_sent_packets);
    anysignal_sharemap_unpack_field(in, out, anylink_uhf_tx_sent_frames);
    anysignal_sharemap_unpack_field(in, out, anylink_uhf_tx_overflow_frames);
    anysignal_sharemap_unpack_field(in, out, anylink_sband_tx_sent_bytes);
    anysignal_sharemap_unpack_field(in, out, anylink_sband_tx_sent_packets);
    anysignal_sharemap_unpack_field(in, out, anylink_sband_tx_sent_frames);
    anysignal_sharemap_unpack_field(in, out, anylink_sband_tx_overflow_frames);
    anysignal_sharemap_unpack_field(in, out, anylink_xband_tx_sent_bytes);
    anysignal_sharemap_unpack_field(in, out, anylink_xband_tx_sent_packets);
    anysignal_sharemap_unpack_field(in, out, anylink_xband_tx_sent_frames);
    anysignal_sharemap_unpack_field(in, out, anylink_xband_tx_overflow_frames);
    anysignal_sharemap_unpack_field(in, out, anylink_sband_rx_received_bytes);
    anysignal_sharemap_unpack_field(in, out, anylink_sband_rx_received_packets);
    anysignal_sharemap_unpack_field(in, out, anylink_sband_rx_received_frames);
    anysignal_sharemap_unpack_field(in, out, anylink_sband_rx_dropped_packets);
    anysignal_sharemap_unpack_field(in, out, anylink_sband_rx_dropped_frames);
    anysignal_sharemap_unpack_field(in, out, anylink_sband_rx_socket_errors);
    anysignal_sharemap_unpack_field(in, out, anylink_sband_rx_idle_frames);
    anysignal_sharemap_unpack_field(in, out, anylink_heartbeats_sent);
    anysignal_sharemap_unpack_field(in, out, anylink_heartbeats_received);
    anysignal_sharemap_unpack_field(in, out, anylink_rx_radio_bad_header);
    anysignal_sharemap_unpack_field(in, out, anylink_rx_radio_packets_received);
    anysignal_sharemap_unpack_field(in, out, anylink_tx_radio_packets_send_errors);
    anysignal_sharemap_unpack_field(in, out, anylink_tx_radio_packets_sent);
    anysignal_sharemap_unpack_field(in, out, anylink_tx_radio_packet_nodest);
    anysignal_sharemap_unpack_field(in, out, anylink_tx_radio_packet_truncate);
    anysignal_sharemap_unpack_field(in, out, anylink_tx_radio_packet_pad);
    anysignal_sharemap_unpack_field(in, out, anylink_rx_radio_no_endpoint);
    anysignal_sharemap_unpack_field(in, out, anylink_rx_radio_reject_echo);
    anysignal_sharemap_unpack_field(in, out, anylink_total_endpoint_packets_received);
    anysignal_sharemap_unpack_field(in, out, anylink_total_endpoint_packets_sent);
    anysignal_sharemap_unpack_field(in, out, anylink_encryption_failed);
    anysignal_sharemap_unpack_field(in, out, anylink_decryption_failed);
    anysignal_sharemap_unpack_field(in, out, anylink_tap_endpoint_active_tx_channel);
    anysignal_sharemap_unpack_field(in, out, anylink_tap_endpoint_mtu);
    anysignal_sharemap_unpack_field(in, out, anylink_tap_endpoint_recv_bytes);
    anysignal_sharemap_unpack_field(in, out, anylink_tap_endpoint_recv_errors);
    anysignal_sharemap_unpack_field(in, out, anylink_tap_endpoint_recv_packets);
    anysignal_sharemap_unpack_field(in, out, anylink_tap_endpoint_send_bytes);
    anysignal_sharemap_unpack_field(in, out, anylink_tap_endpoint_send_errors);
    anysignal_sharemap_unpack_field(in, out, anylink_tap_endpoint_send_packets);
    return out;
}

// Call a templated function on every sharemap
#define anysignal_sharemap_for_each(fcn, ...) {\
        fcn<sharemap_config_t>(__VA_ARGS__); \
        fcn<sharemap_metrics_t>(__VA_ARGS__); \
    }

} // namespace anysignal