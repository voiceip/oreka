## Oreka, an opensource VoIP media capture and retrieval platform

Based on [OrecX](http://www.orecx.com/open-source/) [Oreka](https://github.com/OrecX/Oreka), this project tries to provide a complete Call Recording (SIPREC) solution.  

### Components
- **Orkaudio**:  
    The audio capture and storage daemon with pluggable capture modules currently comes with modules for VoIP and sound device recording.
- **Orktrack**:  
    Tracks and publishes all activity from one or more orkaudio services to any mainstream database/storage system.
- **Orkweb**:   
    Web based user interface for retrieval
    
### Improvements

**OrkAudio** 

- Support for G729 Codec  
- CallID Tracking   
- Upgrade to latest version of libraries.

**OrkTrack**

- Switch to Maven
- CallID Tracking   
- Switch to faster logging (Log4j2)
- Upgrade to java8
- Metrics via [jmx](https://metrics.dropwizard.io/4.1.2/)
- Lombork & Aspectj code weaving for auto code generation and metrics  

**Orkweb**:   
- Switch to Maven
- Call Play / Download on all platform(s)/OS(es)


### Building

#### Docker

```bash
export DOCKER_BUILDKIT=1
distribution/docker
docker build -f Dockerfile.orkaudio -t orkaudio .
docker run -itd --net=host --restart=always --privileged=true -v /var/log/orkaudio:/var/log/orkaudio --name orkaudio orkaudio
```

#### Debian

The build tool is separately available at [github:Oreka-build](https://github.com/voiceip/oreka-build) which builds the project on a Ubuntu14.04 Virtual Box. 
You can natively build if you have all dependencies but I develop on a OSx system, so have kept it separate.

### Distribution & Installation

#### Docker

Docker images are available via docker hub, so just run the below command to pull images directly from hub.docker.com. Note: `--net=host` on docker works on linux systems and is a [limitation of docker](https://docs.docker.com/network/host/), so please keep that in mind.

```bash
docker run -itd --net=host --restart=always --privileged=true -v /var/log/orkaudio:/var/log/orkaudio voiceip/orkaudio
```

#### Debian

Binary releases are available from the [Releases Section](https://github.com/voiceip/oreka/releases). Download and install.

~~Ubuntu installers are available via [Bintray](https://bintray.com/kingster/deb/oreka). To add the sources to your system run the following command~~  _Bintray currenty broken due to some issue. I will soon figure out how to publish with multiple versions._


```bash
#echo "deb https://dl.bintray.com/kingster/deb /" | sudo tee -a /etc/apt/sources.list.d/oreka.list
#apt update && apt install orkaudio
apt install ./orkaudio.deb
svc -u /etc/service/orkaudio #to-start
svc -d /etc/service/orkaudio #to-stop
```



#### More Information
Read Original [Readme](README.txt)
