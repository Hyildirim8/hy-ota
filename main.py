from fastapi import FastAPI, HTTPException
from fastapi.responses import FileResponse
import os

app = FastAPI()

FIRMWARE_PATH = "firmware/firmware.bin"
SIGNATURE_PATH = "firmware/firmware.bin.sig"
PUBLIC_KEY_PATH = "keys/public.pem"
CURRENT_VERSION = "1.0.2"  # versiyon numarası manuel gir

@app.get("/version")
def get_version():
    return {"version": CURRENT_VERSION}

@app.get("/firmware")
def get_firmware():
    if not os.path.exists(FIRMWARE_PATH):
        raise HTTPException(status_code=404, detail="Firmware not found.")
    return FileResponse(FIRMWARE_PATH, media_type='application/octet-stream', filename="firmware.bin")

@app.get("/signature")
def get_signature():
    if not os.path.exists(SIGNATURE_PATH):
        raise HTTPException(status_code=404, detail="Signature not found.")
    return FileResponse(SIGNATURE_PATH, media_type='application/octet-stream', filename="firmware.bin.sig")

@app.get("/public-key")
def get_public_key():
    if not os.path.exists(PUBLIC_KEY_PATH):
        raise HTTPException(status_code=404, detail="Public key not found.")
    return FileResponse(PUBLIC_KEY_PATH, media_type='application/x-pem-file', filename="public.pem")
