FROM ubuntu:focal

ENV DEBIAN_FRONTEND=noninteractive 
RUN apt-get update
RUN apt-get install -y git build-essential cmake
RUN apt-get install -y libboost-all-dev libconfig++-dev libyaml-cpp-dev
RUN apt-get install -y python3-pip python-is-python3 
RUN apt-get install -y gdb

WORKDIR /setup/accelergy
RUN git clone https://github.com/HewlettPackard/cacti.git
RUN git clone https://github.com/Accelergy-Project/accelergy.git
RUN git clone https://github.com/Accelergy-Project/accelergy-aladdin-plug-in.git
RUN git clone https://github.com/Accelergy-Project/accelergy-cacti-plug-in.git
RUN git clone https://github.com/Accelergy-Project/accelergy-table-based-plug-ins.git

# just for mac
# RUN sed -i 's/-m64//g' cacti/cacti.mk
# RUN sed -i 's/-gstabs+//g' cacti/cacti.mk
# end jfm

RUN cd cacti && make -j4
RUN cd accelergy && pip3 install .
RUN cd accelergy-aladdin-plug-in/ && pip3 install .
RUN cd accelergy-cacti-plug-in/ && pip3 install .
RUN cp -r cacti /usr/local/share/accelergy/estimation_plug_ins/accelergy-cacti-plug-in/
RUN cd accelergy-table-based-plug-ins/ && pip3 install .

RUN accelergy
RUN accelergyTables

WORKDIR /setup/
RUN apt-get install -y scons libncurses-dev libtinfo-dev libgpm-dev
RUN git clone https://github.com/NVlabs/timeloop.git
WORKDIR /setup/timeloop/src
RUN ln -s ../pat-public/src/pat .
WORKDIR /setup/timeloop
RUN scons -j2 --accelergy --static

# WORKDIR /lambda/
# RUN git clone https://github.com/Haimrich/timeloop.git
# WORKDIR /lambda/timeloop/src
# RUN ln -s ../pat-public/src/pat .
# WORKDIR /lambda/timeloop
# RUN scons -j2 --accelergy --static

WORKDIR /app/