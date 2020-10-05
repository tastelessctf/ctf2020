FROM python:alpine

ADD requirements.txt /requirements.txt
RUN pip install -r requirements.txt && rm /requirements.txt

ADD chall.py default_settings.py /chall/
ADD templates/ /chall/templates/
WORKDIR /chall
ENV FLASK_APP=chall.py

RUN apk update && apk add openldap-clients
RUN adduser \
    --disabled-password \
    --gecos "" \
    --home "/var/empty" \
    --no-create-home \
    "celery"