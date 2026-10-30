// Contract 09 datasets over live devices (contracts/09_DATASET_FORMAT.md):
// the record value of each sensor's result, and the session recorder that
// syncs every device's clock, hooks its result stream and writes one
// `.depzdata` on a shared host timeline — the Python SDK's SessionRecorder.
// Part of the live-hardware layer (needs depz/device.hpp); the format itself
// (reader, writer, player) is depz/dataset.hpp.
#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include "depz/dataset.hpp"
#include "depz/device.hpp"

namespace depz {
namespace dataset {

// The contract 09 record value (`v`) of each kind, from the device classes'
// results — what SessionRecorder writes, public for writers of your own. Key
// for key the reference Python SDK's values (integers as integers, reals as
// reals, in its key order).
namespace records {

// `sr04`: {echo_us, source: "once" | "loop"}.
Value sr04(const Sr04Measurement& m);
// `vl53l4`: the result fields; distance_mm raw whatever range_status says.
Value vl53l4(const Vl53l4cdMeasurement& m);
// `vl53l8`: resolution, silicon_temp_degc and the per-zone arrays cut to the
// resolution; `layers` adds signal_per_spad, ambient_per_spad, range_sigma_mm
// and reflectance; `motion` when the frame carried the motion-indicator block.
Value vl53l8(const Vl53l8LiveFrame& f, bool layers = false);
// `vl53l8_cnh`: the raw CNH block of a frame, base64, with the armed setup's
// decode parameters (nb_aggregates, feature_length, sum_span, start_bin).
Value vl53l8_cnh(byte_span cnh_raw, const CnhSetup& setup);
// `vl53lx`: product, driver, status, distance_mm, sigma_mm, signal_kcps,
// ambient_kcps, spads, and `targets` when the driver reported several.
Value vl53lx(const Vl53lxMeasurement& m, const std::string& product, const std::string& driver);
// `bno055`: unit_sel and every channel the block covered, scaled.
Value bno055(const Bno055Sample& s);
// `tmf8829`: one measurement — fp_mode, rows, cols, peaks, f_number, flags,
// temperature_c, distance_mm (mm, rounded to 0.1, flat [peak][row][col]),
// snr, and signal / noise / xtalk when the result format carried them.
Value tmf8829(const Tmf8829Measurement& m);
// `bno086`: the report in snake_case — type and sensor id, then only the
// fields the report carries (scaled: x/y/z, i/j/k/real, accuracy_rad, bias,
// angular_velocity, value), the detector outputs, the integer counts of the
// raw ADC streams (0x14..0x16), and the device timestamp.
Value bno086(const Bno086Report& r);
// `temperature`: {celsius, source} — source "mcu" or "sensor".
Value temperature(double celsius, const std::string& source);

// Python's round(x, 1): the nearest tenth of the exact binary value, ties to even.
double round1(double x);
// Standard base64 with padding.
std::string base64(byte_span data);

}  // namespace records

// Record decoded data from several devices onto one host timeline. add()
// each open device (it runs Device::sync_time() and hooks the device's result
// stream), then start(): a record per result at device time − offset, written
// on the device's reader thread. The recorder never reconfigures a device —
// start and stop streaming yourself. stop() (or the destructor) closes the
// file. A plain Device records nothing.
// SessionRecorder construction options.
struct RecorderOptions {
    std::string note;                          // free text in the header
    bool vl53l8_layers = false;                // VL53L8 records also carry signal / ambient / sigma / reflectance
    std::optional<double> temperature_hz = 1;  // about this many `temperature` records per device and
                                               // second (the MCU thermometer, and the sensor die where
                                               // frames carry one); nullopt for none
};

class SessionRecorder {
public:
    using Options = RecorderOptions;

    // A recorder writing `path` (`.depzdata`, or `.depzdata.gz`).
    explicit SessionRecorder(const std::string& path, RecorderOptions options = RecorderOptions());
    ~SessionRecorder();  // stops
    SessionRecorder(const SessionRecorder&) = delete;
    SessionRecorder& operator=(const SessionRecorder&) = delete;

    // Register a device before recording starts (after: ArgumentError, the class of
    // DEPZ_E_ARG, which serves invalid_state — contract 07 §3):
    // runs sync_time with `sync_samples` rounds so its timestamps land on the
    // shared timeline, reads its serial and software name, hooks its stream.
    // Returns the device id (`device_id`, or d0, d1, … in add order). The
    // device must outlive the recorder.
    std::string add(Device& device, std::optional<std::string> device_id = std::nullopt, int sync_samples = 5);
    // Open the file and start writing. No-op while recording;
    // std::runtime_error when the file cannot be created.
    void start();
    // Stop writing, unhook the devices and close the file. Idempotent.
    void stop();
    bool recording() const;
    // Records in the file being written; after stop(), in the finished one.
    std::uint64_t records_written() const;

private:
    struct Impl;
    std::shared_ptr<Impl> impl_;
};

}  // namespace dataset
}  // namespace depz
