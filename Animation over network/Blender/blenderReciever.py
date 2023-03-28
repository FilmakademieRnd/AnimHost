import bpy
import zmq
import threading
import mathutils
import math

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



def decode_quaternion(utf8_bytes):
    vector_string = utf8_bytes.decode('utf-8')
    vector_string = vector_string.strip('()')  # Remove the parentheses
    x, y, z, w = map(float, vector_string.split(','))
    return mathutils.Quaternion((w, x, y, z))

# Set the IP address and port number from the publisher script
ip = "127.0.0.1"
port = "5555"
context = zmq.Context()


def blender_receiver():
    socket = context.socket(zmq.SUB)
    socket.setsockopt_string(zmq.SUBSCRIBE, "foo")
    for unity_bone_name in unity_to_blender_bone_mapping:
        socket.setsockopt_string(zmq.SUBSCRIBE, unity_bone_name)

    socket.bind("tcp://{}:{}".format(ip, port))

    print("Subscriber connected to: tcp://{}:{}\nWaiting for data...".format(ip, port))

    # Continuously receive and process data from the publisher
    while True:
        topic, message = socket.recv_multipart()
        # print("Received message on topic {}: {}".format(topic, message))

        # Decode the received quaternion message
        quaternion = decode_quaternion(message)
        newQuatRot = mathutils.Quaternion((quaternion.w, quaternion.x, -quaternion.y, -quaternion.z))

        # ROOT
        if topic == b"foo":
            # ADD math.radians(90) on the x axis
            bpy.data.objects['Armature'].rotation_mode = 'QUATERNION'
            bpy.data.objects['Armature'].rotation_quaternion = newQuatRot
            bpy.data.objects['Armature'].rotation_mode = 'XYZ'
            bpy.data.objects['Armature'].rotation_euler.rotate_axis("X", math.radians(90))

        # BONES
        bone_name = topic.decode('utf-8')  # Convert the topic byte string to a regular string
        if bone_name in unity_to_blender_bone_mapping:
            blender_bone_name = unity_to_blender_bone_mapping[bone_name]

            # Apply the received rotation to the corresponding bone in Blender
            armature = bpy.data.objects['Armature']
            if blender_bone_name in armature.pose.bones:
                blender_bone = armature.pose.bones[blender_bone_name]


                blender_bone.rotation_mode = 'XYZ'

                blender_bone.rotation_euler = newQuatRot.to_euler()

                blender_bone.rotation_mode = 'XYZ'



# Start the receiver in a separate thread
receiver_thread = threading.Thread(target=blender_receiver)
receiver_thread.daemon = True
receiver_thread.start()