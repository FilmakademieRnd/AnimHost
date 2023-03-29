import bpy
import zmq
import threading
import mathutils
import math

# Define a dictionary that maps Unity bone names to Mixamo bone names
unity_to_blender_bone_mapping = {
    # //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-BODY+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
    # //body/body
    "Hips": "mixamorig:Hips",
    "Spine": "mixamorig:Spine",
    "Chest": "mixamorig:Spine1",
    "UpperChest": "mixamorig:Spine2",

    # //body/Left Arm
    "LeftShoulder": "mixamorig:LeftShoulder",
    "LeftUpperArm": "mixamorig:LeftArm",
    "LeftLowerArm": "mixamorig:LeftForeArm",
    "LeftHand": "mixamorig:LeftHand",

    # //body/Right Arm
    "RightShoulder": "mixamorig:RightShoulder",
    "RightUpperArm": "mixamorig:RightArm",
    "RightLowerArm": "mixamorig:RightForeArm",
    "RightHand": "mixamorig:RightHand",

    # //body/Left Leg
    "LeftUpperLeg": "mixamorig:LeftUpLeg",
    "LeftLowerLeg": "mixamorig:LeftLeg",
    "LeftFoot": "mixamorig:LeftFoot",
    "LeftToes": "mixamorig:LeftToeBase",

    # //body/Right Leg
    "RightUpperLeg": "mixamorig:RightUpLeg",
    "RightLowerLeg": "mixamorig:RightLeg",
    "RightFoot": "mixamorig:RightFoot",
    "RightToes": "mixamorig:RightToeBase",
    # //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+/BODY+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-

    # //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-HEAD+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
    # //Head/Head
    "Neck": "mixamorig:Neck",
    "Head": "mixamorig:Head",
    # //{"Left Eye", "mixamorig:something"}
    # //{"Right Eye", "mixamorig:something"}
    # //{"Jaw", "mixamorig:something"}

    # //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-/HEAD+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-

    # //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-LEFT HAND+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
    "LeftThumbProximal": "mixamorig:LeftHandThumb1",
    "LeftThumbIntermediate": "mixamorig:LeftHandThumb2",
    "LeftThumbDistal": "mixamorig:LeftHandThumb3",
    "LeftIndexProximal": "mixamorig:LeftHandIndex1",
    "LeftIndexIntermediate": "mixamorig:LeftHandIndex2",
    "LeftIndexDistal": "mixamorig:LeftHandIndex3",
    "LeftMiddleProximal": "mixamorig:LeftHandMiddle1",
    "LeftMiddleIntermediate": "mixamorig:LeftHandMiddle2",
    "LeftMiddleDistal": "mixamorig:LeftHandMiddle3",
    "LeftRingProximal": "mixamorig:LeftHandRing1",
    "LeftRingIntermediate": "mixamorig:LeftHandRing2",
    "LeftRingDistal": "mixamorig:LeftHandRing3",
    "LeftLittleProximal": "mixamorig:LeftHandPinky1",
    "LeftLittleIntermediate": "mixamorig:LeftHandPinky2",
    "LeftLittleDistal": "mixamorig:LeftHandPinky3",

    # //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+/LEFT HAND+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-

    # //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-RIGHT HAND+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
    "RightThumbDistal": "mixamorig:RightHandThumb3",
    "RightIndexProximal": "mixamorig:RightHandIndex1",
    "RightIndexIntermediate": "mixamorig:RightHandIndex2",
    "RightIndexDistal": "mixamorig:RightHandIndex3",
    "RightMiddleProximal": "mixamorig:RightHandMiddle1",
    "RightMiddleIntermediate": "mixamorig:RightHandMiddle2",
    "RightMiddleDistal": "mixamorig:RightHandMiddle3",
    "RightRingProximal": "mixamorig:RightHandRing1",
    "RightRingIntermediate": "mixamorig:RightHandRing2",
    "RightRingDistal": "mixamorig:RightHandRing3",
    "RightLittleProximal": "mixamorig:RightHandPinky1",
    "RightLittleIntermediate": "mixamorig:RightHandPinky2",
    "RightLittleDistal": "mixamorig:RightHandPinky3"
}



# Define function to decode quaternion from UTF-8 encoded byte string
def decode_quaternion(utf8_bytes):
    vector_string = utf8_bytes.decode('utf-8')  # Decode byte string to UTF-8 string
    vector_string = vector_string.strip('()')  # Remove parentheses from string
    x, y, z, w = map(float, vector_string.split(','))  # Split string into float values and store in variables
    return mathutils.Quaternion((w, x, y, z))  # Return Quaternion object created from float values

# Define function to decode vector from UTF-8 encoded byte string
def decode_transform(utf8_bytes):
    vector_string = utf8_bytes.decode('utf-8')  # Decode byte string to UTF-8 string
    vector_string = vector_string.strip('()')  # Remove parentheses from string
    x, y, z = map(float, vector_string.split(','))  # Split string into float values and store in variables
    return mathutils.Vector((x, y, z))  # Return Vector object created from float values

# Set the IP address and port number from the publisher script
ip = "127.0.0.1"
port = "5555"
context = zmq.Context()  # Create ZeroMQ context object

# Define function to receive and process data from publisher
def blender_receiver():
    socket = context.socket(zmq.SUB)  # Create ZeroMQ subscriber socket
    socket.setsockopt_string(zmq.SUBSCRIBE, "foo")  # Subscribe to "foo" topic
    socket.setsockopt_string(zmq.SUBSCRIBE, "transform")  # Subscribe to "transform" topic
    for unity_bone_name in unity_to_blender_bone_mapping:  # Subscribe to bone names specified in unity_to_blender_bone_mapping
        socket.setsockopt_string(zmq.SUBSCRIBE, unity_bone_name)

    socket.bind("tcp://{}:{}".format(ip, port))  # Bind socket to specified IP address and port number

    print("Subscriber connected to: tcp://{}:{}\nWaiting for data...".format(ip, port))

    # Continuously receive and process data from the publisher
    while True:
        topic, message = socket.recv_multipart()  # Receive data as a topic and message pair

        if topic == b"transform":  # If the topic is "transform"
            transform = decode_transform(message)  # Decode the message into a vector
            # Set the location of the "Armature" object in Blender based on the received vector
            bpy.data.objects['Armature'].location.x = -transform.x
            bpy.data.objects['Armature'].location.y = -transform.z
            bpy.data.objects['Armature'].location.z = transform.y

        else:
            # If the topic is not "transform", decode the message into a quaternion
            quaternion = decode_quaternion(message)
            # Create a new quaternion with different axis order and orientation to match Blender's coordinate system
            newQuatRot = mathutils.Quaternion((quaternion.w, quaternion.x, -quaternion.y, -quaternion.z))

        # If the topic is "foo"
        if topic == b"foo":
            # Create a new quaternion with different axis order and orientation to match Blender's coordinate system
            newQuatRot = mathutils.Quaternion((quaternion.w, quaternion.x, -quaternion.z, -quaternion.y))
            bpy.data.objects['Armature'].rotation_mode = 'QUATERNION'  # Set rotation mode to quaternion
            bpy.data.objects['Armature'].rotation_quaternion = newQuatRot  # Set the rotation of the "Armature" object in Blender to the new quaternion
            bpy.data.objects['Armature'].rotation_mode = 'XYZ'  # Set rotation mode to Euler angles
            bpy.data.objects['Armature'].rotation_euler.rotate_axis("X", math.radians(90))  # Rotate 90 degrees around the X axis

        # If the topic is a bone name specified in unity_to_blender_bone_mapping
        bone_name = topic.decode('utf-8')  # Convert the topic byte string to a regular string
        if bone_name in unity_to_blender_bone_mapping:
            blender_bone_name = unity_to_blender_bone_mapping[bone_name]  # Get the corresponding Blender bone name from unity_to_blender_bone_mapping

            # Apply the received rotation to the corresponding bone in Blender
            armature = bpy.data.objects['Armature']  # Get the "Armature" object in Blender
            if blender_bone_name in armature.pose.bones:  # Check if the corresponding bone exists in the armature
                blender_bone = armature.pose.bones[blender_bone_name]  # Get the corresponding bone from the armature

                blender_bone.rotation_mode = 'QUATERNION'  # Set rotation mode to quaternion
                blender_bone.rotation_quaternion = newQuatRot  # Set the rotation of the bone to the new quaternion

# Start the receiver in a separate thread
receiver_thread = threading.Thread(target=blender_receiver)  # Create a new thread that runs the blender_receiver function
receiver_thread.daemon = True  # Set the thread to run as a daemon, so that it will automatically stop when the main program stops
receiver_thread.start()  # Start the thread