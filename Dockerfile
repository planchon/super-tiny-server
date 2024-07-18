FROM gcc:latest

# Set the working directory in the container
WORKDIR /usr/src/app

# Copy the current directory contents into the container at /usr/src/app
COPY . .

# Compile the C server program
RUN gcc sts.c -std=gnu11 -o sts

# Make port 8080 available to the world outside this container
EXPOSE 9191

CMD ["./sts"]
