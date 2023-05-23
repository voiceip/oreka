## Oreka, an opensource VoIP media capture and retrieval platform

Based on [OrecX](http://www.orecx.com/open-source/) [Oreka](https://github.com/OrecX/Oreka), this project tries to provide a complete Call Recording (SIPREC) solution.  

[![Build Status](https://travis-ci.com/voiceip/oreka.svg?branch=master)](https://travis-ci.com/voiceip/oreka)


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
- SIP-CallID Tracking   
- Upgrade to latest version of libraries.
- [Realtime Streaming](https://github.com/voiceip/oreka/wiki/Live-Streaming) of Live Calls.

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
docker build -f Dockerfile.orkaudio -t voiceip/orkaudio .
docker run -it --net=host --restart=always --privileged=true -v /var/log/orkaudio:/var/log/orkaudio  -v /etc/orkaudio:/etc/orkaudio voiceip/orkaudio:latest 
```

#### Debian

The build tool is separately available at [github:Oreka-build](https://github.com/voiceip/oreka-build) which builds the project on a Ubuntu14.04 Virtual Box. 
You can natively build if you have all dependencies but I develop on a OSx system, so have kept it separate.

### Distribution & Installation

#### Docker

Docker images are available via [docker hub](https://hub.docker.com/r/voiceip/orkaudio/tags), so just run the below command to pull images directly from hub.docker.com. Note: `--net=host` on docker works on linux systems and is a [limitation of docker](https://docs.docker.com/network/host/), so please keep that in mind.

```bash
docker run -it --net=host --restart=always --privileged=true -v /var/log/orkaudio:/var/log/orkaudio -v /etc/orkaudio:/etc/orkaudio voiceip/orkaudio:latest
```

#### Debian/Ubuntu

Binary releases are available from the [Releases Section](https://github.com/voiceip/oreka/releases). Download and refer to the [installation instructions](https://github.com/voiceip/oreka/wiki/Installation).


#### More Information
Read Original [Readme](README.txt)
