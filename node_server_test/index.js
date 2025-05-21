const net = require('net');

const PORT = 8888;

let totalBytes = 0;
let startTime = null;

const server = net.createServer((socket) => {
  console.log('Client connected:', socket.remoteAddress);

  totalBytes = 0;
  startTime = process.hrtime();

  socket.on('data', (data) => {
    totalBytes += data.length;
  });

  socket.on('end', () => {
    const diff = process.hrtime(startTime);
    const seconds = diff[0] + diff[1] / 1e9;
    const mbits = (totalBytes * 8) / 1e6; // bits to megabits
    const speedMbps = mbits / seconds;

    console.log(`Connection closed. Received ${totalBytes} bytes.`);
    console.log(`Elapsed time: ${seconds.toFixed(3)} seconds.`);
    console.log(`Average speed: ${speedMbps.toFixed(3)} Mbps.`);
  });

  socket.on('error', (err) => {
    console.error('Socket error:', err);
  });
});

server.listen(PORT, () => {
  console.log(`TCP Server listening on port ${PORT}`);
});
