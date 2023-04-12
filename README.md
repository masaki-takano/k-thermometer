# K型温度計

- BLE で温度を送信する。

- MAX6675
  https://datasheets.maximintegrated.com/en/ds/MAX6675.pdf

- ESP32 SPI
  - https://trac.switch-science.com/wiki/esp32_tips
  - https://trac.switch-science.com/wiki/esp32_tips#SPI%E9%80%9A%E4%BF%A1

- ESP32 I2C
  - https://trac.switch-science.com/wiki/esp32_tips#SPI%E9%80%9A%E4%BF%A1


# グラフ

```
python -m SimpleHTTPServer
```

```
pyenv local 3.7.4
python -m http.server
```

http://127.0.0.1:8000/chart.html
