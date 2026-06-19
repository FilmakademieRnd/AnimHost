using System;
using System.Collections.Generic;
using UnityEngine;
using NetMQ;
using NetMQ.Sockets;
using System.Text;

public class Publisher : MonoBehaviour
{
    // IP address and port to connect to
    public string ip = "127.0.0.1";
    public string port = "5555";

    private PublisherSocket socket;
    private string topic = "foo";
    private string topicBones = "bones";
    private int i = 0;
    private bool on = true;
    private Quaternion previousRotation;
    private Vector3 previousPos;
    private Quaternion blenderInitialRotation;
    private Quaternion armatureOffset;

    Animator animator;

    [SerializeField] private GameObject Model;
    [SerializeField] private GameObject Hips;

    public List<Quaternion> boneOffsets;

    void Start()
    {
        // ZMQ connection setup
        // Create a string URL based on the IP address and port number
        string url = "tcp://" + ip + ":" + port;
        // Create a new publisher socket to connect to the URL
        socket = new PublisherSocket();
        socket.Connect(url); // publisher connects to subscriber
        // Print a debug message indicating that the publisher has connected to the URL
        Debug.Log("Pub connected to: " + url + "\nSending data...");

        // If the Model object is not null, get its animator component and previous rotation and position
        if (Model != null)
        {
            animator = Model.GetComponent<Animator>();
            previousRotation = Model.transform.rotation;
            previousPos = Model.transform.position;
        }

        // Set the blenderInitialRotation quaternion to zero rotation
        blenderInitialRotation = new Quaternion(0, 0, 0, 1);
        // Calculate the armature offset quaternion based on the model's current rotation
        armatureOffset = GetOffsetQuaternion(Model.transform.rotation, new Quaternion(0.707f, 0.707f, 0f, 0f));
        // Calculate the offset quaternion for each bone in the animator
        boneOffsets = GetAllBonesOffsetQuaternions(animator, blenderInitialRotation);
    }


    // Define a method that retrieves the offset quaternion for each bone in an animator
    private List<Quaternion> GetAllBonesOffsetQuaternions(Animator animator, Quaternion blenderInitialRotation)
    {
        // Create an empty list to store the offset quaternions for each bone
        List<Quaternion> boneOffsets = new List<Quaternion>();

        // Loop through each bone in the HumanBodyBones enum, except the last one (which is not valid)
        foreach (HumanBodyBones bone in Enum.GetValues(typeof(HumanBodyBones)))
        {
            if (bone == HumanBodyBones.LastBone) continue;

            // Get the transform for the current bone in the animator
            Transform boneTransform = animator.GetBoneTransform(bone);

            // If the bone has a valid transform, get its offset quaternion and add it to the list
            if (boneTransform != null)
            {
                // Calculate the offset quaternion for the bone using the GetOffsetQuaternion method
                Quaternion offsetQuaternion = GetOffsetQuaternion(boneTransform.localRotation, blenderInitialRotation);
                // Add the offset quaternion to the boneOffsets list
                boneOffsets.Add(offsetQuaternion);
            }
            // If the bone does not have a valid transform, add the identity quaternion to the list
            else
            {
                boneOffsets.Add(Quaternion.identity);
            }
        }

        // Return the list of offset quaternions for each bone
        return boneOffsets;
    }

    void Update()
    {
        // If the model's rotation or position has changed since the last frame, send the data
        if (Model.transform.rotation != previousRotation || Model.transform.position != previousPos)
        {
            SendData();
            // Update the previousRotation and previousPos variables to the current values
            previousRotation = Model.transform.rotation;
            previousPos = Model.transform.position;
        }
    }


    // Define a method that sends data over the ZMQ connection
    void SendData()
    {
        // If the "on" boolean is true, continue sending messages over the ZMQ connection
        if (on)
        {
            // Send the model's rotation as a string using the specified topic
            byte[] topicBytes = Encoding.ASCII.GetBytes(topic);
            byte[] msgBytes = Encoding.UTF8.GetBytes(Model.transform.rotation.ToString());
            socket.SendMoreFrame(topicBytes).SendFrame(msgBytes);

            // Send the model's position as a string using a separate "transform" topic
            byte[] positiontopicBytes = Encoding.ASCII.GetBytes("transform");
            byte[] positionmsgBytes = Encoding.UTF8.GetBytes(Model.transform.position.ToString());
            socket.SendMoreFrame(positiontopicBytes).SendFrame(positionmsgBytes);

            // Loop through each bone in the HumanBodyBones enum, except the last one
            foreach (HumanBodyBones bone in Enum.GetValues(typeof(HumanBodyBones)))
            {
                if (bone != HumanBodyBones.LastBone)
                {
                    // Get the index and offset quaternion for the current bone
                    int boneIndex = (int)bone;
                    Quaternion boneOffset = boneOffsets[boneIndex];

                    // Initialize a new quaternion variable to store the bone's rotation with the offset applied
                    var sendmsg = new Quaternion();
                    // Get the transform for the current bone in the animator
                    var boneTransform = animator.GetBoneTransform(bone);
                    // If the bone has a valid transform, apply the offset quaternion to its local rotation
                    if (boneTransform != null)
                    {
                        sendmsg = ApplyOffsetQuaternion(boneOffset, boneTransform.localRotation);
                        // If the bone is the left or right upper leg, invert the Z component of the localRotation
                        if (bone == HumanBodyBones.LeftUpperLeg || bone == HumanBodyBones.RightUpperLeg)
                        {
                            sendmsg.x = -sendmsg.x;
                        }

                        // Send the bone's rotation as a string using the bone's name as the topic
                        byte[] bonesTopic = Encoding.ASCII.GetBytes(bone.ToString());
                        byte[] boneTrans = Encoding.UTF8.GetBytes((sendmsg).ToString());
                        socket.SendMoreFrame(bonesTopic).SendFrame(boneTrans);
                    }
                }
            }
        }
    }

    // Define a method that calculates the offset quaternion between two rotations
    public Quaternion GetOffsetQuaternion(Quaternion unityInitialRotation, Quaternion blenderInitialRotation)
    {
        // Calculate the offset quaternion by multiplying the blenderInitialRotation by the inverse of the unityInitialRotation
        Quaternion offsetQuaternion = blenderInitialRotation * Quaternion.Inverse(unityInitialRotation);
        return offsetQuaternion;
    }

    // Define a method that applies an offset quaternion to a transform quaternion and returns the resulting quaternion
    public static Quaternion ApplyOffsetQuaternion(Quaternion transformQuaternion, Quaternion offsetQuaternion)
    {
        // Calculate the blenderQuaternionWithOffset by multiplying the offsetQuaternion by the transformQuaternion
        Quaternion blenderQuaternionWithOffset = offsetQuaternion * transformQuaternion;
        // Normalize the resulting quaternion and return it
        return blenderQuaternionWithOffset.normalized;
    }

    // This method is called when the script is destroyed
    public void OnDestroy()
    {
        // Disconnect, close, and dispose of the ZMQ socket and clean up NetMQ
        socket.Disconnect("tcp://" + ip + ":" + port);
        socket.Close();
        socket.Dispose();
        NetMQConfig.Cleanup(false);
        // Set the "on" boolean to false
        on = false;
    }

}


