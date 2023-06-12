# TFTP Client

## Usage

```
tftp_client host [GET | PUT] source destination [--transfer_mode [netascii | octet]] [--option_name name --option_value value]...
```

| | |
| ----------- | ------------------------------------------------------------------------------ |
| host | Specifies the local or remote host |
| GET | Transfers the file source on the remote host to the file destination on the local host |
| PUT | Transfers the file source on the local host to the file destination on the remote host |
| source | Specifies the file to transfer |
| destination | Specifies where to transfer the file |
| transfer_mode | Specifies the transfer mode, possible values: netascii, octet |
| option_name | Specifies the name of the option, may be repeated |
| option_value | Specifies the value of the option, may be repeated |

## Implemented features ([RFC 1350](https://datatracker.ietf.org/doc/html/rfc1350))

| Feature | Implementation status |
| ----------- | ----------------- |
| READ requests | ${\color{green}\text{Done}}$ |
| Write requests | ${\color{green}\text{Done}}$ |

## Extensions support

| Extension | Implementation status |
| ----------- | ----------------- |
| [RFC 2347: TFTP Option Extension](https://datatracker.ietf.org/doc/html/rfc2347) | ${\color{green}\text{Done}}$ |
| [RFC 2348: TFTP Blocksize Option](https://datatracker.ietf.org/doc/html/rfc2348) | ${\color{red}\text{TODO}}$ |
| [RFC 2349: TFTP Timeout Interval and Transfer Size Options](https://datatracker.ietf.org/doc/html/rfc2349) | ${\color{red}\text{TODO}}$ |
| [RFC 7440: TFTP Windowsize Option](https://datatracker.ietf.org/doc/html/rfc7440) | ${\color{red}\text{TODO}}$ |
