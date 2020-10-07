FROM golang:1.15 AS builder

COPY . /b

RUN cd /b && CGO_ENABLED=0 go build -o /app .

FROM scratch

COPY --from=builder /app /app
ADD server.crt /
ADD server.key /
CMD [ "/app" ]
