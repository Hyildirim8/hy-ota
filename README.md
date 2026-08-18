# OTA Server

RSA imzalı firmware dağıtımı yapan basit bir FastAPI OTA (Over-The-Air) güncelleme sunucusu. ESP32/ESP-IDF cihazların firmware güncellemelerini imza doğrulaması ile indirmesi için tasarlanmıştır.

## Proje Yapısı

- `main.py` — FastAPI sunucusu (`/version`, `/firmware`, `/signature`, `/public-key` endpoint'leri)
- `keys/` — RSA anahtar çifti üretimi (`anahtar_uret.py`)
- `firmware/` — dağıtılacak `firmware.bin` ve imzalama betiği (`imzala.py`)
- `ota-app/` — ESP-IDF istemci uygulaması
- `test_ota_server.py` — sunucuyu test eden istemci betiği (firmware indirip imza doğrular)

## Kurulum

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

## Anahtar Üretimi

```bash
cd keys
python3 anahtar_uret.py
```

`private.pem` gizli tutulmalı, `public.pem` cihazlara dağıtılır.

## Firmware İmzalama

`firmware.bin` dosyasını `firmware/` klasörüne koyup imzalayın:

```bash
cd firmware
python3 imzala.py
```

## Sunucuyu Çalıştırma

```bash
uvicorn main:app --host 0.0.0.0 --port 8000 --reload
```

`main.py` içindeki `CURRENT_VERSION` değişkenini her yeni sürümde manuel güncelleyin.

## Test

```bash
python3 test_ota_server.py
```

`test_ota_server.py` içindeki `BASE_URL` değerini sunucu adresinize göre düzenleyin.

## Endpoint'ler

| Endpoint | Açıklama |
|---|---|
| `GET /version` | Güncel firmware versiyonunu döner |
| `GET /firmware` | `firmware.bin` dosyasını indirir |
| `GET /signature` | Firmware imzasını indirir |
| `GET /public-key` | Doğrulama için public key'i indirir |
