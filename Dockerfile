FROM python:3.12-slim
WORKDIR /app
RUN apt-get update \
    && apt-get install --no-install-recommends -y iputils-ping \
    && rm -rf /var/lib/apt/lists/*
COPY app.py .
COPY static ./static
COPY start.sh .
RUN chmod 755 start.sh
USER nobody
EXPOSE 8080
ENTRYPOINT ["./start.sh"]
