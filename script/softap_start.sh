sudo ifdown wlan0

mv -f /etc/network/interfaces /etc/network/interfaces.bak
cp interfaces.softap /etc/network/interfaces

mv -f/etc/hostapd/hostapd.conf /etc/hostapd/hostapd.conf.bak
cp hostapd.conf /etc/hostapd/hostapd.conf

mv -f /etc/dhcp/dhcpd.conf /etc/dhcp/dhcpd.conf.bak
cp dhcpd.config /etc/dhcp/dhcpd.conf

sudo ifup wlan0
sudo ifconfig wlan0 192.168.42.1

sudo service hostapd start 
sudo service isc-dhcp-server start

