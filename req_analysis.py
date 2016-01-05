import mysql.connector    
import os
import atexit

FIFO = 'IPs.fifo'

cnx = mysql.connector.connect(user='analyser', password='1234567890',
                              host='127.0.0.1',
                              database='analysis')

try:
	cursor = cnx.cursor()
	cursor.execute("""    
	CREATE TABLE IF NOT EXISTS requests
	(
	  id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
	  ip VARCHAR (128) UNIQUE NOT NULL,
	  lastest_port INT NOT NULL,
	  lastest_req TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
	  num_requests INT NOT NULL
	);
   """)

	if os.path.exists("IPs.fifo"):
		os.remove("IPs.fifo")

	os.mkfifo(FIFO)
	while True:
		fifo = open(FIFO, "rw")
		for line in fifo:
			ip = line.split(":")
			raw_ip = ip[0]
			raw_port = ip[1]
			print "IP: "+raw_ip+" Port: "+raw_port
			query = ("INSERT INTO requests (ip, lastest_port, lastest_req, num_requests) VALUES (%s, %s, NOW(), %s) ON DUPLICATE KEY UPDATE lastest_port = %s, lastest_req = NOW(), t num_requests = num_requests + 1")
			cursor.execute(query, (raw_ip, int(raw_port), 1, int(raw_port)))
			cnx.commit()
		fifo.close()
finally:
	cnx.close()
