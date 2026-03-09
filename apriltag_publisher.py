import rclpy
from rclpy.node import Node
from std_msgs.msg import Int32MultiArray

class AprilTagPublisher(Node):

    def __init__(self):
        super().__init__('apriltag_publisher')
        self.publisher = self.create_publisher(
            Int32MultiArray,
            'apriltag_data',
            10
        )

    def publish_tag(self, tag_id):

        tag_str = f"{tag_id:05d}"

        values = [
            int(tag_str[0]),
            int(tag_str[1]),
            int(tag_str[2]),
            int(tag_str[3]),
            int(tag_str[4])
        ]

        msg = Int32MultiArray()
        msg.data = values

        self.publisher.publish(msg)

        self.get_logger().info(f"Send: {values}")


def main():

    rclpy.init()
    node = AprilTagPublisher()

    tag_id = 12345   # example from detector

    node.publish_tag(tag_id)

    rclpy.spin(node)

    node.destroy_node()
    rclpy.shutdown()
