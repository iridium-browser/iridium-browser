# Generating Certificates

## Name Constraints Examples

The following commands were used along with `extensions.conf` to generate the
certificates in `nc.pem` and `nc_fail.pem`.

``` bash
# Once for each certificate.
$ openssl genrsa -out keyN.pem 2048
$ openssl req -new -key keyN.pem -out certN.csr

# <extension> will be v3_ca_nc for the intermediate and v3_req for the device.
$ openssl x509 -req -in certN.csr -CA certN-1.pem -CAkey keyN-1.pem
    -CAcreateserial -extensions <extension> -extfile extensions.conf -out
    certN.pem -days 365 -sha256
```

Note: it looks like `openssl req` also accepts extensions via `-reqexts` but
there is a known bug in openssl where extensions are transferred between CSRs
and X509 certs.
