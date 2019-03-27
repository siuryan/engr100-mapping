import csv

class LogReader:

    def __init__(self):
        self.logs = {}

    def read_logs(self, filename, headers=False):
        with open(filename) as csv_file:
            reader = csv.reader(csv_file)
            if headers:
                next(reader, None)

            for row in reader:
                self.logs[int(row[0])] = {
                    'time': int(row[0]),
                    'l1': float(row[1]),
                    'l2': float(row[2]),
                    'l3': float(row[3]),
                    'l4': float(row[4]),
                    'a_x': float(row[5]),
                    'a_y': float(row[6]),
                    'roll': float(row[7]),
                    'pitch': float(row[8])
                }

    def get_logs(self):
        return self.logs
