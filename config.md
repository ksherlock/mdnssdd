
# Introduction

mdnssdd uses the filesystem (directories, files, and their contents) to hold configuration information. 

# Details

the config directory holds 0 or more child directories, each one corresponding to a service to be broadcasted.  The child directory name is for your convenience.  Each child directory is treated as a dictionary, with 0 or more UTF-8 keys (files) with a value (file contents).  Leading and trailing whitespace characters are stripped from the values.

Refer to documentation for DNSServiceRegister for more specific information and length limits.

## Keys


### domain

optional.  Default value is the default domain(s).  
 
### hostname 
optional.  Default value is the default host name.  May be used to advertise services on other computers (?)

### name
optional.  Default value is the name of the computer.

### type
required.  The service type and protocol, formatted as `'_' <type> '.' '_' <protocol> (',' '_' <subtype> )*`

 * type: 1-14 letters, digits, or hyphens.   See  [this list](http://www.dns-sd.org/ServiceTypes.html).
 * protocol: tcp or udp
 * subtype: any additional (and optional) subtype information.

Examples:

 * `_telnet._tcp`
 * `_nfs._udp`
 * `_http._tcp,_apache`
 
### port
optional, but useful.  The port number for the service.  Specifying a value of 0  (the default value) prevents this type of service from being broadcast.

### txt 
optional.  Any additional  DNS TXT record rdata.
