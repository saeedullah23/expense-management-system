FROM ubuntu:latest

# Install C++ compiler and tools
RUN apt-get update && apt-get install -y g++ make

WORKDIR /app

# Copy project files
COPY . .

# Compile C++ application
RUN g++ -std=c++17 -O2 -o ems main.cpp Utils.cpp User.cpp Expense.cpp Budget.cpp FileManager.cpp ExpenseManager.cpp Report.cpp WebServer.cpp

# Expose port 8080
EXPOSE 8080

# Run Web Server mode
CMD ["./ems", "--web", "8080"]
