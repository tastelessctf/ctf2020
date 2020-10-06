FROM ubuntu

RUN apt-get update && apt-get -y install build-essential python3
ADD ./chall.py /chall.py
# The setarch call did not work inside docker, so we strip it and disable
# ASLR on the complete deployment node instead. :/
# Probably, there is a better, CAP based solution, but hey, whatever works, right?
RUN sed -i "s/RUN_CMD = \['setarch', 'x86_64', '-R', '\.\/a\.out'\]/RUN_CMD=['.\/a.out']/" ./chall.py
ADD ./flag.txt /flag.txt
ENTRYPOINT [ "/chall.py" ]



