# Pairtun

transforming simpletun in [link](http://backreference.org/2010/03/26/tuntap-interface-tutorial/) for different purpose.

## create tun iface

* `sudo openvpn --mktun --dev tun0`

* `sudo ip addr add 10.0.0.1/24 dev tun0`

* `sudo ip link set tun0 up`
