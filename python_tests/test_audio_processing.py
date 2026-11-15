"""Regression and contract tests for Eyesore's offline audio processing."""
import numpy as np
import pytest

from eyesore_audio import processing as P


SR = 48_000
FRAME_MS = 40.0
HOP_MS = 10.0
BOUNDARY_TOLERANCE_MS = FRAME_MS - HOP_MS
SAMPLE_PEAK_CEILING = 0.999
SAMPLE_PEAK_TOLERANCE = 1e-6


def tone(hz=220.0, seconds=1.0, amp=0.3):
    t = np.arange(int(SR * seconds), dtype=np.float64) / SR
    return (amp * np.sin(2.0 * np.pi * hz * t)).astype(np.float32)


def integrated_tone(frequency, seconds):
    t = np.arange(int(SR * seconds), dtype=np.float64) / SR
    phase = 2.0 * np.pi * np.cumsum(frequency(t)) / SR
    return (0.3 * np.sin(phase)).astype(np.float32), t


def glide_fixture(start_hz, end_hz, seconds=2.0):
    slope = (end_hz - start_hz) / seconds
    audio, _ = integrated_tone(lambda t: start_hz + slope * t, seconds)
    return audio, lambda t: start_hz + slope * t


def cents_error(estimated_hz, expected_hz):
    return 1200.0 * np.log2(estimated_hz / expected_hz)


def test_f0_tracks_tone():
    track = P.track_f0(tone(), SR)
    assert track.voiced.sum() > 20
    assert np.median(track.f0_hz[track.voiced]) == pytest.approx(220, abs=4)


def test_silence_unvoiced():
    track = P.track_f0(np.zeros(SR), SR)
    assert not track.voiced.any()
    assert np.max(track.confidence) == 0


@pytest.mark.parametrize("start_hz,end_hz,direction", [
    (100.0, 400.0, "ascending"),
    (400.0, 100.0, "descending"),
])
def test_f0_glide_frame_accuracy_and_octave_errors(start_hz, end_hz, direction):
    audio, truth = glide_fixture(start_hz, end_hz)
    track = P.track_f0(
        audio, SR, min_f0_hz=65.0, max_f0_hz=500.0,
        frame_ms=FRAME_MS, hop_ms=HOP_MS,
    )
    valid = track.voiced & (track.frame_times_seconds > 0.10) & (
        track.frame_times_seconds < 1.90
    )
    assert np.count_nonzero(valid) > 100, direction
    expected = truth(track.frame_times_seconds[valid])
    cents = cents_error(track.f0_hz[valid], expected)
    octave_errors = np.abs(np.abs(cents) - 1200.0) <= 100.0
    assert np.count_nonzero(octave_errors) == 0, direction
    # This is the same 100-cent frame-level MAE contract used by ElevenX PR #26.
    assert float(np.mean(np.abs(cents))) <= 100.0, direction


def test_f0_confidence_is_bounded_and_informative():
    clean = tone(220.0, seconds=1.0, amp=0.3)
    silence = np.zeros_like(clean)
    noise = np.random.default_rng(20261114).normal(
        0.0, 0.3, clean.size
    ).astype(np.float32)
    clean_track = P.track_f0(clean, SR)
    silence_track = P.track_f0(silence, SR)
    noise_track = P.track_f0(noise, SR)

    for track in (clean_track, silence_track, noise_track):
        assert np.isfinite(track.confidence).all()
        assert np.all(track.confidence >= 0.0)
        assert np.all(track.confidence <= 1.0)

    clean_voiced = clean_track.confidence[clean_track.voiced]
    assert clean_voiced.size > 20
    assert float(np.median(clean_voiced)) >= 0.60
    assert float(np.max(silence_track.confidence)) <= 0.10
    assert float(np.median(noise_track.confidence)) < 0.60


def test_f0_vibrato_tracks_defined_220_hz_modulation():
    duration = 4.0
    audio, time = integrated_tone(
        lambda t: 220.0 * np.power(
            2.0,
            (15.0 * np.sin(2.0 * np.pi * 5.5 * t)) / 1200.0,
        ),
        duration,
    )
    track = P.track_f0(audio, SR)
    valid = track.voiced & (track.frame_times_seconds > 0.2) & (
        track.frame_times_seconds < duration - 0.2
    )
    assert np.count_nonzero(valid) > 100
    expected = 220.0 * np.power(
        2.0,
        (15.0 * np.sin(2.0 * np.pi * 5.5 * track.frame_times_seconds[valid])) / 1200.0,
    )
    errors = cents_error(track.f0_hz[valid], expected)
    # Matches the 100-cent frame-level MAE bound in ElevenX PR #26.
    assert float(np.mean(np.abs(errors))) <= 100.0


@pytest.mark.parametrize("transition", ["silence_to_voice", "voice_to_silence"])
def test_voicing_transition_respects_window_geometry(transition):
    duration = 1.0
    boundary = 0.5
    time = np.arange(int(duration * SR), dtype=np.float64) / SR
    voiced_audio = 0.3 * np.sin(2.0 * np.pi * 220.0 * time)
    if transition == "silence_to_voice":
        audio = np.where(time >= boundary, voiced_audio, 0.0)
        side = "start"
    else:
        audio = np.where(time < boundary, voiced_audio, 0.0)
        side = "end"

    track = P.track_f0(audio.astype(np.float32), SR)
    voiced_times = track.frame_times_seconds[track.voiced]
    assert voiced_times.size > 0
    observed_edge = float(voiced_times[0] if side == "start" else voiced_times[-1])
    error_ms = abs(observed_edge - boundary) * 1000.0
    assert error_ms <= BOUNDARY_TOLERANCE_MS + 1e-3


def test_compressor_bypass_is_bit_identical():
    audio = tone(220.0, amp=0.8)
    output = P.compress_vocal(
        audio, SR, P.CompressorSettings(ratio=1.0, makeup_db=0.0)
    )
    assert np.array_equal(output, audio.astype(np.float32))


@pytest.mark.parametrize("bad_value", [np.nan, np.inf, -np.inf])
def test_compressor_rejects_nonfinite_even_in_bypass(bad_value):
    audio = tone()
    audio[10] = bad_value
    with pytest.raises(ValueError, match="finite"):
        P.compress_vocal(
            audio, SR, P.CompressorSettings(ratio=1.0, makeup_db=0.0)
        )


def test_float32_max_boundary_is_accepted():
    audio = np.array([np.finfo(np.float32).max], dtype=np.float64)
    output = P.compress_vocal(
        audio, SR, P.CompressorSettings(ratio=1.0, makeup_db=0.0)
    )
    assert np.isfinite(output).all()
    assert output[0] == np.finfo(np.float32).max


@pytest.mark.parametrize("value", [3.5e38, 1e300, -3.5e38, -1e300])
def test_values_outside_float32_range_are_rejected(value):
    audio = np.array([value], dtype=np.float64)
    assert np.isfinite(audio).all()
    with pytest.raises(ValueError, match="overflows float32"):
        P.compress_vocal(
            audio, SR, P.CompressorSettings(ratio=1.0, makeup_db=0.0)
        )


def test_compressor_ceiling_is_enforced_after_makeup():
    audio = tone(220.0, amp=0.99)
    settings = P.CompressorSettings(ratio=1.0, makeup_db=0.1)
    output = P.compress_vocal(audio, SR, settings)
    assert np.isfinite(output).all()
    assert float(np.max(np.abs(output))) <= SAMPLE_PEAK_CEILING + SAMPLE_PEAK_TOLERANCE


def test_compressor_accepts_finite_above_full_scale_and_caps_output():
    audio = tone(220.0, amp=1.001)
    output = P.compress_vocal(
        audio, SR, P.CompressorSettings(ratio=1.0, makeup_db=0.01)
    )
    assert np.isfinite(output).all()
    assert float(np.max(np.abs(output))) <= SAMPLE_PEAK_CEILING + SAMPLE_PEAK_TOLERANCE


@pytest.mark.parametrize("function", [P.track_f0, P.compress_vocal])
@pytest.mark.parametrize("bad_value", [np.nan, np.inf, -np.inf])
def test_audio_processing_rejects_nonfinite_input(function, bad_value):
    audio = tone()
    audio[0] = bad_value
    with pytest.raises(ValueError, match="finite"):
        function(audio, SR)


def test_combined_pipeline():
    pitch, output = P.process_local_vocal(tone(), SR)
    assert pitch.f0_hz.size
    assert output.size == SR
    assert np.isfinite(output).all()
