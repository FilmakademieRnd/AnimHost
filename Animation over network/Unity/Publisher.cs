using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using NetMQ;
using NetMQ.Sockets;
using System.Text;
using System.Threading;
using UnityEngine.Serialization;

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
    private Quaternion blenderInitialRotation;
    Animator animator;

    [SerializeField] private GameObject Model;

    public List<Quaternion> boneOffsets;
    
    // Start is called before the first frame update
    void Start()
    {
        // ZMQ connection
        string url = "tcp://" + ip + ":" + port;
        socket = new PublisherSocket();
        socket.Connect(url); // publisher connects to subscriber
        Debug.Log("Pub connected to: " + url + "\nSending data...");
        
        if (Model != null)
        {
            animator = Model.GetComponent<Animator>();
            previousRotation = Model.transform.rotation;
        }

        blenderInitialRotation = new Quaternion(0, 0, 0, 1);
        boneOffsets = GetAllBonesOffsetQuaternions(animator, blenderInitialRotation);

    }
    
    private List<Quaternion> GetAllBonesOffsetQuaternions(Animator animator, Quaternion blenderInitialRotation)
    {
        List<Quaternion> boneOffsets = new List<Quaternion>();

        foreach (HumanBodyBones bone in Enum.GetValues(typeof(HumanBodyBones)))
        {
            if (bone == HumanBodyBones.LastBone) continue;

            Transform boneTransform = animator.GetBoneTransform(bone);
            if (boneTransform != null)
            {
                Quaternion offsetQuaternion = GetOffsetQuaternion(boneTransform.localRotation, blenderInitialRotation);
                boneOffsets.Add(offsetQuaternion);
            }
            else
            {
                boneOffsets.Add(Quaternion.identity);
            }
        }

        return boneOffsets;
    }
    
    
    // Update is called once per frame
    void Update()
    {
        if (Model.transform.rotation != previousRotation)
        {
            SendShit();
            previousRotation = Model.transform.rotation;
        }
        
    }

    void SendShit()
    {
        if (on)
        {
            // Keep sending messages until program interruption
            byte[] topicBytes = Encoding.ASCII.GetBytes(topic);
            byte[] msgBytes = Encoding.UTF8.GetBytes(Model.transform.rotation.ToString());

            // Publish data
            socket.SendMoreFrame(topicBytes).SendFrame(msgBytes);
            Debug.Log("On topic " + topic + ", send data: " + Model.transform.rotation);



            foreach (HumanBodyBones bone in Enum.GetValues(typeof(HumanBodyBones)))
            {
                if (bone != HumanBodyBones.LastBone)
                {
                    int boneIndex = (int)bone;
                    Quaternion boneOffset = boneOffsets[boneIndex];

                    var sendmsg = new Quaternion();
                    var boneTransform = animator.GetBoneTransform(bone);
                    if (boneTransform != null)
                    {
                        sendmsg = ApplyOffsetQuaternion(boneOffset, boneTransform.localRotation);
                        Debug.LogError("boneOffset: " + boneOffset + "  localRotation: " + boneTransform.localRotation +
                                       "  sendmsg: " + sendmsg);
                        byte[] bonesTopic = Encoding.ASCII.GetBytes(bone.ToString());
                        byte[] boneTrans = Encoding.UTF8.GetBytes((sendmsg).ToString());
                        socket.SendMoreFrame(bonesTopic).SendFrame(boneTrans);
                        Debug.Log("On topic " + bone.ToString() + ", send data Eulrt: " +
                                  boneTransform.localRotation.ToEuler() + "Quaternion Local: " +
                                  boneTransform.localRotation + "Quaternion rotation: " + boneTransform.rotation +
                                  "the shit we send: " + (sendmsg).ToEuler());
                    }
                }
            }
        }
    }


    public Quaternion GetOffsetQuaternion(Quaternion unityInitialRotation, Quaternion blenderInitialRotation)
    {
        Quaternion offsetQuaternion = blenderInitialRotation * Quaternion.Inverse(unityInitialRotation);
        return offsetQuaternion;
    }
    
    public static Quaternion ApplyOffsetQuaternion(Quaternion transformQuaternion, Quaternion offsetQuaternion)
    {
        Quaternion blenderQuaternionWithOffset = offsetQuaternion * transformQuaternion;
        return blenderQuaternionWithOffset;
    }
    
    public void OnDestroy()
    {
        socket.Disconnect("tcp://" + ip + ":" + port);
        socket.Close();
        socket.Dispose();
        NetMQConfig.Cleanup(false);
        on = false;
    }
}


