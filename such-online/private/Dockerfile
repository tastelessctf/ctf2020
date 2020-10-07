FROM golang:1.15 AS builder

COPY . /a

WORKDIR /a

RUN CGO_ENABLED=0 go build -o /app .

FROM scratch

COPY --from=builder /app /app
COPY maze.png /
COPY index.html /

CMD [ "/app" ]