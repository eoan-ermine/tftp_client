# TFTP Client

## Реализованные возможности ([RFC 1350](https://tools.ietf.org/html/rfc1350))

| Возможность | Статус реализации |
| ----------- | ----------------- |
| READ запросы | ${\color{orange}\text{Частично реализовано}}$ ([см. прим. 1](#примечания)) |
| WRITE запросы | ${\color{orange}\text{Частично реализовано}}$ ([см. прим. 1](#примечания)) |

### Примечания

1. Не реализована обработка повторных пакетов от сервера 

## Поддержка расширений

| Расширение | Статус реализации |
| ----------- | ----------------- |
| [RFC 2347: TFTP Option Extension](https://www.rfc-editor.org/rfc/rfc2347) | ${\color{red}\text{Не реализовано}}$ |
| [RFC 2348: TFTP Blocksize Option](https://www.rfc-editor.org/rfc/rfc2347) | ${\color{red}\text{Не реализовано}}$ |
| [RFC 2349: TFTP Timeout Interval and Transfer Size Options](https://www.rfc-editor.org/rfc/rfc2347) | ${\color{red}\text{Не реализовано}}$ |
