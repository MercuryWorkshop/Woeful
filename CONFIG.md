# Woeful Configuration

Basic config, run with `--config basic.xml`
```xml
<?xml version="1.0" encoding="UTF-8"?>
<config block_type="black">
  <port port_number="9001" />
  <pcap pcap_capture="false" />

  <!--Blacklisting targets--> 
  <list_hostname hostname="example.com" />
  <list_port port="22" />
</config>
```
