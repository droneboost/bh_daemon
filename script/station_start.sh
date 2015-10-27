#this script enable station mode for bithollow
#TODO: replace service with systemctl
sudo ifdown wlan0
#sudo ifconfig wlan0 down
sudo killall wpa_supplicant
sudo killall hostapd
sudo service isc-dhcp-server stop
sudo service lighttpd stop

cp -f /etc/network/interfaces /etc/network/interfaces.bak
cp -f config/interfaces.station /etc/network/interfaces

cp -f /etc/wpa_supplicant.conf /etc/wpa_supplicant.conf.bak
cp -f config/wpa_supplicant.conf /etc/wpa_supplicant.conf

sudo ifup wlan0
sudo service lighttpd start
