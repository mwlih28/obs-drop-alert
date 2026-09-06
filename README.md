# OBS Drop Uyarısı (`obs-drop-alert`)

Yayın sırasında OBS drop yapmaya başladığında OBS penceresini kırmızıya boyayan,
yanıp söndüren, ses çalan ve görev çubuğunu flaşlatan bir OBS eklentisi.

Amaç basit: drop olduğunu anlamak için Stats penceresini açıp bakmak zorunda
kalmamak. Oyun oynarken OBS arka planda kaldığında drop dakikalarca fark
edilmeden sürebiliyor — bu eklentide uyarı kendisi dikkat çekiyor.

## Ne izliyor

Dört sorun türü, her biri ayrı ayrı açılıp kapatılabilir ve kendi eşiği var:

| Sorun | Kaynak | Ne zaman olur |
|---|---|---|
| Ağ drop'u | yayın çıktısının düşen kareleri | yükleme bant genişliği yetmediğinde |
| Render lag | `obs_get_lagged_frames()` | GPU sahneyi zamanında oluşturamadığında |
| Encoder overload | `video_output_get_skipped_frames()` | encoder ayarı CPU'ya ağır geldiğinde |
| Kayıt / disk | kayıt çıktısının düşen kareleri + boş disk alanı | disk yetişemediğinde veya dolmak üzereyken |

### Neden kayan pencere

OBS'in Stats penceresinde gösterdiği yüzdeler **kümülatiftir**: iki saatlik bir
yayında on saniyelik bir drop patlaması toplam yüzdeyi zar zor kıpırdatır. Canlı
uyarı için bu işe yaramaz.

Bu eklenti bunun yerine son N saniyedeki (varsayılan 5) sayaç **artışını** ölçer,
yani "şu anda drop var mı" sorusuna cevap verir. Eşiğin tam sınırında alarmın
titrememesi için histerezis uygulanır: alarm üst üste K örnek (varsayılan 2)
aşımdan sonra başlar, M saniye (varsayılan 3) temiz geçince söner.

## Uyarı biçimleri

Hepsi Araçlar → **Drop Uyarısı Ayarları** altından ayarlanabilir:

- **Kırmızı kenarlık** — OBS penceresinin çevresinde kalın kırmızı çerçeve,
  üst ortada sorunun adını ve oranını gösteren bir rozet
  (örn. `ENCODER OVERLOAD (CPU) 8.40%`).
- **Tüm pencere kırmızı** — kenarlığa ek olarak tüm pencereye ayarlanabilir
  yoğunlukta kırmızı yıkama.
- **Yanıp sönme** — sabit renk yerine ayarlanabilir hızda nabız.
- **Uyarı sesi** — eklentiyle gelen `alert.wav` ya da kendi seçtiğin bir WAV;
  istenirse alarm sürdükçe belirli aralıkla tekrarlar.
- **Görev çubuğu flaşı** — OBS başka pencerenin arkasındayken görev çubuğundaki
  OBS düğmesi yanıp söner.

**Test uyarısı** düğmesi gerçek bir drop beklemeden tüm zinciri çalıştırır ve
penceredeki güncel (henüz kaydedilmemiş) değerleri kullanır, böylece görünümü
canlı ayarlayabilirsin.

### Uyarı yalnızca OBS arayüzünde görünür

Kırmızı katman ayrı bir üst düzey pencere olarak OBS'in üstünde çizilir; yayına
veya kayda **hiçbir şekilde karışmaz**. Ana pencerenin içine konan normal bir Qt
widget'ı OBS'in önizleme alanının altında kalırdı (önizleme native bir alt pencere
kullanır ve native pencereler kardeş Qt widget'larının her zaman üstüne çizilir),
bu yüzden ana pencereye sahipli bir `Qt::Tool` penceresi tercih edildi. Bu sayede
katman her şeyin üstünde durur, OBS ile birlikte gizlenir, başka uygulamaların
önüne geçmez.

## Derleme (Windows)

Gerekenler: Visual Studio 2022 Build Tools (C++ iş yükü), Windows SDK, CMake 3.28+.
Qt ve libobs bağımlılıkları configure sırasında otomatik indirilir.

```bash
cmake --preset windows-x64
```

```bash
cmake --build --preset windows-x64 --config RelWithDebInfo --parallel
```

```bash
cmake --install build_x64 --prefix release/RelWithDebInfo --config RelWithDebInfo
```

> `.github/scripts/Build-Windows.ps1` kullanılmıyor: PowerShell 7 ve `CI` ortam
> değişkeni istiyor, ayrıca uyarıları hata sayan CI presetini seçiyor.

## Kurulum

`release/RelWithDebInfo/obs-drop-alert` klasörünü olduğu gibi şuraya kopyala
(yönetici hakkı gerekmez):

```
%APPDATA%\obs-studio\plugins\
```

Sonuç şöyle görünmeli:

```
%APPDATA%\obs-studio\plugins\obs-drop-alert\bin\64bit\obs-drop-alert.dll
%APPDATA%\obs-studio\plugins\obs-drop-alert\data\alert.wav
%APPDATA%\obs-studio\plugins\obs-drop-alert\data\locale\*.ini
```

OBS'i yeniden başlat. Araçlar menüsünde **Drop Uyarısı Ayarları** görünüyorsa
eklenti yüklenmiştir.

## Ayarlar nerede tutuluyor

```
%APPDATA%\obs-studio\plugin_config\obs-drop-alert\config.json
```

OBS'in frontend config API'si sürümler arasında değiştiği için (`..._global_config`
→ `..._user_config`) bilinçli olarak kullanılmadı; kendi JSON dosyamız sürümden
bağımsız çalışır.

## Sürüm notu

Eklenti OBS 31.1.1 başlıklarına karşı derlenir ama OBS 32.x üzerinde çalışır:
libobs yalnızca **daha yeni** libobs ile derlenmiş modülleri reddeder
(`obs_open_module`: `if (ver > LIBOBS_API_VER)`), daha eskiyi kabul eder. Qt de
6.x serisi içinde ileri yönde ikili uyumludur.

## Kaynak düzeni

```
src/plugin-main.cpp       modül girişi, Araçlar menüsü, parçaları birbirine bağlar
src/DropMonitor.*         sayaç okuma, kayan pencere, histerezis
src/AlertOverlay.*        kırmızı katman, nabız, sebep rozeti
src/Alerter.*             uyarı sesi (winmm) + görev çubuğu flaşı (FlashWindowEx)
src/Settings.*            JSON ayar yükleme/kaydetme
src/SettingsDialog.*      Araçlar menüsünden açılan ayar penceresi
data/locale/*.ini         tr-TR ve en-US metinleri
data/alert.wav            varsayılan uyarı sesi
```

## Lisans

GPL-2.0-or-later (OBS eklentileri libobs'a bağlandığı için GPL olmak zorunda).
