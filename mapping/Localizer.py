class Localizer:

    def __init__(self, logs, wall_threshold, accel_threshold):
        self.WALL_THRESHOLD = wall_threshold
        self.ACCEL_THRESHOLD = accel_threshold

        self.logs = logs
        self.walls = []  # List of tuples

        self.position_x = []
        self.position_y = []

        self.velocity_x = []
        self.velocity_y = []

        self._populate_velocity(logs)
        self._populate_position(logs)
        self._populate_walls(logs)


    def _populate_velocity(self, logs):
        self.velocity_x.append(0)
        self.velocity_y.append(0)

        for i in range(len(logs)-1):
            a_x = logs[i]['a_x'] # TODO: use angles
            a_y = logs[i]['a_y'] # TODO: use angles

            if a_x < self.ACCEL_THRESHOLD:
                a_x = 0
            if a_y < self.ACCEL_THRESHOLD:
                a_y = 0

            t1 = logs[i]['time']
            t2 = logs[i+1]['time']
            v_x = a_x*(t2 - t1) + self.velocity_x[i]
            v_y = a_y*(t2 - t1) + self.velocity_y[i]
            self.velocity_x.append(v_x)
            self.velocity_y.append(v_y)

    def _populate_position(self, logs):
        self.position_x.append(0)
        self.position_y.append(0)

        for i in range(len(logs)-1):
            v_x = self.velocity_x[i]
            v_y = self.velocity_y[i]
            t1 = logs[i]['time']
            t2 = logs[i+1]['time']
            d_x = v_x*(t2 - t1) + self.position_x[i]
            d_y = v_y*(t2 - t1) + self.position_y[i]
            self.position_x.append(d_x)
            self.position_y.append(d_y)

    def _populate_walls(self, logs):
        for i in range(len(logs)):
            # TODO: use angles
            # up
            if logs[i]['l1'] < self.WALL_THRESHOLD:
                self.walls.append((self.position_x[i], self.position_y[i] + logs[i]['l1']))
            # right
            if logs[i]['l2'] < self.WALL_THRESHOLD:
                self.walls.append((self.position_x[i] + logs[i]['l2'], self.position_y[i]))
            # down
            if logs[i]['l3'] < self.WALL_THRESHOLD:
                self.walls.append((self.position_x[i], self.position_y[i] - logs[i]['l3']))
            # left
            if logs[i]['l4'] < self.WALL_THRESHOLD:
                self.walls.append((self.position_x[i] - logs[i]['l4'], self.position_y[i]))

    def get_walls(self):
        return self.walls

    def get_positions_x(self):
        return self.position_x

    def get_positions_y(self):
        return self.position_y
