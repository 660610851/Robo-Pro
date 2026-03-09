import rclpy
from rclpy.node import Node
from std_msgs.msg import Int32MultiArray


class MotionController(Node):

    def __init__(self):

        super().__init__('motion_controller')

        self.subscription = self.create_subscription(
            Int32MultiArray,
            'apriltag_data',
            self.callback,
            10
        )

        self.ticks_per_cm = 40

    def move_distance(self, cm):

        target_ticks = cm * self.ticks_per_cm

        print("Move", cm, "cm")

        while True:

            encoder_value = self.read_encoder()

            if encoder_value >= target_ticks:
                self.stop_motor()
                break

            self.forward()

    def callback(self, msg):

        A, B, C, D, E = msg.data

        dist1 = A
        dist2 = B
        dist3 = C * 5
        dist4 = D
        dist5 = E

        print("Received:", A,B,C,D,E)

        self.move_distance(dist1)
        self.move_distance(dist2)
        self.move_distance(dist3)
        self.move_distance(dist4)
        self.move_distance(dist5)

    def read_encoder(self):
        return 0

    def forward(self):
        pass

    def stop_motor(self):
        pass


def main():

    rclpy.init()
    node = MotionController()

    rclpy.spin(node)

    node.destroy_node()
    rclpy.shutdown()
