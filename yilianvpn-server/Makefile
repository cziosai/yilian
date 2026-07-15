ALL : app

clean:
	rm -f app

app : Main.cpp VpnServer.h Client.h \
  Proxy.h TcpProxy.h UdpProxy.h Config.h \
  CommonMethods.h IPHeader.h TCPHeader.h ICMPHeader.h \
  UDPHeader.h Task.h Socket.h \
  ServerSocket.h
	g++ -g -rdynamic Main.cpp VpnServer.h Client.h \
  Proxy.h TcpProxy.h UdpProxy.h Config.h \
  CommonMethods.h IPHeader.h TCPHeader.h ICMPHeader.h \
  UDPHeader.h Task.h Socket.h \
  ServerSocket.h -o app -I./