# sign_firmware.py
from Crypto.Signature import pkcs1_15
from Crypto.Hash import SHA256
from Crypto.PublicKey import RSA

with open("firmware.bin", "rb") as f:
    firmware = f.read()

key = RSA.import_key(open("private.pem").read())
h = SHA256.new(firmware)
signature = pkcs1_15.new(key).sign(h)

with open("firmware.bin.sig", "wb") as f:
    f.write(signature)
