sudo ifdown wlan0

mv -f /etc/network/interfaces /etc/network/interfaces.bak
cp interfaces.station /etc/network/interfaces 

mv -f /etc/wpa_supplicant.conf /etc/wpa_supplicant.conf.bak
cp wpa_supplicant.conf /etc/wpa_supplicant.conf


sudo ifup wlan0
