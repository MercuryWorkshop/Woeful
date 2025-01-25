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

For more complex blocking needs (ie. removing adult content) see [/etc/hosts](https://github.com/StevenBlack/hosts) changing your dns to [1.1.1.3](https://one.one.one.one/family/). As a good rule of thumb you should also consider blocking any hostname or ip that is linked to your local network.
