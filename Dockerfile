FROM ubuntu 
USER root
ENV AP /data/app
ADD a.out $AP/
RUN apt-get -y update
RUN DEBIAN_FRONTEND=noninteractive apt-get -y install libcurl4-openssl-dev
RUN DEBIAN_FRONTEND=noninteractive apt-get -y install libpq5
RUN DEBIAN_FRONTEND=noninteractive apt-get -y install libjansson4
WORKDIR $AP
CMD ["./a.out"]
