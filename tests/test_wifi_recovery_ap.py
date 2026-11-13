from pathlib import Path
s = (Path(__file__).resolve().parents[1] / 'firmware/src/main.cpp').read_text()
def test_recovery_is_not_persisted():
    assert 'bool recoveryApActive = false;' in s
    assert 'if(apEnabled || recoveryApActive)' in s
    assert 'prefs.putBool("apenable",apEnabled)' in s
    assert 'prefs.putBool("apenable",recoveryApActive)' not in s
def test_recovery_timeout_and_reset():
    assert 'kRecoveryApDelayMs = 60000UL' in s
    assert 'recoveryApActive = true;' in s
    assert 'recoveryApActive = false;' in s
    assert 'Recovery AP enabled: Wi-Fi unavailable' in s
