FROM osixia/openldap

RUN apt-get update && apt-get install -y build-essential
ADD flag.txt /flag.txt
RUN chmod 400 flag.txt && chown root:root /flag.txt
ADD flaggetter.c /
RUN gcc -o /flaggetter /flaggetter.c
RUN chmod u+s /flaggetter