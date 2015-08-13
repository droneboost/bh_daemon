sudo ifdown wlan0
killall wpa_supplicant
sudo service hostapd stop
sudo service isc-dhcp-server stop

cp -f /etc/network/interfaces /etc/network/interfaces.bak
cp -f config/interfaces.station /etc/network/interfaces

cp -f /etc/wpa_supplicant.conf /etc/wpa_supplicant.conf.bak
cp -f config/wpa_supplicant.conf /etc/wpa_supplicant.conf

sudo ifup wlan0
