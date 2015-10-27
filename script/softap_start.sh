#this script enable softap mode for bithollow
#TODO: replace service with systemctl
#sleep 10
sudo killall wpa_supplicant
sudo killall hostapd
sudo service isc-dhcp-server stop
sudo service lighttpd stop
sudo systemctl stop ardupilot
sudo ifconfig wlan0 down

cp -f /etc/network/interfaces /etc/network/interfaces.bak
cp -f config/interfaces.softap /etc/network/interfaces

cp -f /etc/hostapd/hostapd.conf /etc/hostapd/hostapd.conf.bak
cp -f config/hostapd.conf /etc/hostapd/hostapd.conf

cp -f /etc/dhcp/dhcpd.conf /etc/dhcp/dhcpd.conf.bak
cp -f config/dhcpd.conf /etc/dhcp/dhcpd.conf

sudo ifconfig wlan0 192.168.42.1 netmask 255.255.255.0 up
sleep 3

sudo  /usr/local/bin/hostapd /etc/hostapd/hostapd.conf -B
sudo service isc-dhcp-server start
sudo service lighttpd start
sudo systemctl start ardupilot
