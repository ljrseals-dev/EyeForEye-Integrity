import numpy as np
import pytest
from eyesore_audio import track_f0, compress_vocal, CompressorSettings, process_local_vocal
SR=48000

def tone(hz=220, seconds=1, amp=.3):
    t=np.arange(int(SR*seconds))/SR
    return (amp*np.sin(2*np.pi*hz*t)).astype(np.float32)

def test_f0_tracks_tone():
    p=track_f0(tone(), SR); assert p.voiced.sum()>20
    assert np.median(p.f0_hz[p.voiced]) == pytest.approx(220, abs=4)

def test_silence_unvoiced():
    p=track_f0(np.zeros(SR), SR); assert not p.voiced.any()

def test_rejects_nonfinite():
    x=tone(); x[0]=np.nan
    with pytest.raises(ValueError): track_f0(x, SR)

def test_compressor_bypass_exact():
    x=tone(); y=compress_vocal(x, SR, CompressorSettings(ratio=1, makeup_db=0))
    assert np.array_equal(x,y)

def test_compressor_peak_bounded():
    y=compress_vocal(tone(amp=1.2), SR, CompressorSettings(ratio=1, makeup_db=1))
    assert np.max(np.abs(y)) <= .99901

def test_combined_pipeline():
    pitch, audio=process_local_vocal(tone(), SR)
    assert pitch.f0_hz.size and audio.size==SR and np.isfinite(audio).all()
