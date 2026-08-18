import requests
from Crypto.Signature import pkcs1_15
from Crypto.Hash import SHA256
from Crypto.PublicKey import RSA

# Sunucu adresi (gerekirse değiştir)
BASE_URL = "http://10.42.101.19:8000"

# 1. Versiyonu çek
def fetch_version():
    r = requests.get(f"{BASE_URL}/version")
    r.raise_for_status()
    version = r.json()['version']
    print("Versiyon:", version)
    return version

# 2. Firmware indir
def fetch_firmware():
    r = requests.get(f"{BASE_URL}/firmware")
    r.raise_for_status()
    with open("test_firmware.bin", "wb") as f:
        f.write(r.content)
    print("Firmware indirildi")
    return "test_firmware.bin"

# 3. İmza indir
def fetch_signature():
    r = requests.get(f"{BASE_URL}/signature")
    r.raise_for_status()
    with open("test_firmware.bin.sig", "wb") as f:
        f.write(r.content)
    print("İmza indirildi")
    return "test_firmware.bin.sig"

# 4. Public key indir
def fetch_public_key():
    r = requests.get(f"{BASE_URL}/public-key")
    r.raise_for_status()
    with open("test_public.pem", "wb") as f:
        f.write(r.content)
    print("Public key indirildi")
    return "test_public.pem"

# 5. İmzayı doğrula
def verify_signature(firmware_path, signature_path, pubkey_path):
    with open(firmware_path, "rb") as f:
        firmware = f.read()
    with open(signature_path, "rb") as f:
        signature = f.read()
    with open(pubkey_path, "rb") as f:
        pubkey = RSA.import_key(f.read())
    h = SHA256.new(firmware)
    try:
        pkcs1_15.new(pubkey).verify(h, signature)
        print("✅ İmza doğrulandı! Firmware geçerli.")
    except (ValueError, TypeError):
        print("❌ İmza HATALI! Firmware güvenli değil.")

if __name__ == "__main__":
    fetch_version()
    fw = fetch_firmware()
    sig = fetch_signature()
    pub = fetch_public_key()
    verify_signature(fw, sig, pub)
