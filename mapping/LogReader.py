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
                self.logs[row[0]] = {
                    'time': row[0],
                    'l1': row[1],
                    'l2': row[2],
                    'l3': row[3],
                    'l4': row[4],
                    'a_x': row[5],
                    'a_y': row[6],
                    'roll': row[7],
                    'pitch': row[8]
                }

    def get_logs(self):
        return self.logs
