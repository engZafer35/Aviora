# Protocol OrionTLV — Web Test Server

Web arayüzü (`React` + `Tailwind`) ve aynı davranışa sahip **Node.js TCP + WebSocket köprüsü** ile `ProtocolOrionTLV_TestServer.py` (Tk) ile eşdeğer Orion TLV test sunucusu.

Tarayıcı doğrudan TCP dinleyemeyeceği için gerçek push/pull trafiği **`server/index.mjs`** içinde çalışır; UI yalnızca WebSocket üzerinden komut ve log alışverişi yapar.

## Gereksinimler

- Node.js 18+

## Kurulum

```bash
cd Application/AppZMeterGw/Services/Protocol/Test_Server/ProtocolOrionWeb
npm install
```

## Geliştirme (önerilen)

İki süreç birden: WebSocket sunucusu (8788) + Vite (5173, `/ws` proxy).

```bash
npm run dev
```

- Yerel: <http://127.0.0.1:5173/> — WebSocket `5173` üzerinden `/ws` ile köprüye proxy edilir.
- Köprü (HTTP+WS): **8788** (`0.0.0.0` — tüm ağ arayüzleri).

### VirtualBox: Linux’tan Windows’taki siteye (ör. 192.168.56.1)

Host-Only ağda Windows genelde **192.168.56.1**, Linux konuk genelde **192.168.56.101** (VirtualBox’ta gördüğünüz IP’yi kullanın).

1. **Windows’ta** `npm run dev` çalışsın (Vite `host: true` ile artık LAN’a açık).
2. **Linux’ta** tarayıcı: `http://192.168.56.1:5173/` — arayüz ve WebSocket aynı origin üzerinden çalışır.
3. **Gateway / test istemcisi Linux’ta** ve push’u Windows’a atıyorsa: hedef **`192.168.56.1:8723`** (UI’da push portunu başlatın; varsayılan 8723).
4. **Pull** (Windows → Linux cihaz dinlemesi): sol panelde **Pull IP** = Linux konuğun IP’si (ör. `192.168.56.101`), **Pull Port** = cihazdaki pull portu (ör. 2622).
5. **Windows Güvenlik Duvarı**: Gelen bağlantılarda Node.js’e izin verin veya **5173**, **8788**, **8723** (TCP) kurallarını “Özel ağ” için açın.

Tek port kullanmak isterseniz: `npm run build && npm start` → yalnız **8788** (statik + `/ws`); Linux’tan `http://192.168.56.1:8788/` açın.

## Sadece köprü (API test)

```bash
npm run server
```

## Üretim benzeri (tek port)

```bash
npm run build
npm start
```

`dist/` üretildikten sonra `node server/index.mjs` statik dosyaları 8788 üzerinden sunar ve `/ws` ile WebSocket açar.

## Cihaz bağlantısı

- Gateway **push** ile bu makineye bağlanır: `npm run dev` iken push portu (varsayılan **8723**) UI’dan başlatılır.
- **Pull** adresi Ident veya sol paneldeki Pull IP/Port ile belirlenir (Python sürümü ile aynı mantık).

Log dosyası `server/ba_server_log_*.txt` altında oluşturulur.

## Tk sürümü ile farklar

- Arayüz web tabanlıdır; hafif animasyon ve gradient kullanılır.
- Köprü Node `net` + `ws`; Python `socket` + Tk yerine geçer.
- Protokol TLV mantığı `server/protocol.mjs` içinde Python ile hizalanmıştır.
