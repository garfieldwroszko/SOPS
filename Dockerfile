FROM gcc:latest

WORKDIR /app

COPY . .

RUN make

CMD ["./czytelnicy_pierwsi"]
