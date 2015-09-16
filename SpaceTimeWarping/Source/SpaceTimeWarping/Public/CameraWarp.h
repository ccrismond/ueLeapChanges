// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "CameraWarp.generated.h"

UCLASS()
class SPACETIMEWARPING_API ACameraWarp : public AActor
{
	GENERATED_BODY()
	
public:	
	protected LeapImageRetriever imageRetriever;
	protected HandController handController;

	protected Transform leftCamera;
	protected Transform rightCamera;
	protected Transform centerCamera;

	public List<LeapImageBasedMaterial> warpedImages;

	public float tweenRewind = 0f;
	public float tweenTimeWarp = 0f;
	public float tweenPosition = 1f;
	public float tweenForward = 1f;

	protected KeyCode unlockHold = KeyCode.RightShift;
	protected KeyCode moreRewind = KeyCode.LeftArrow;
	protected KeyCode lessRewind = KeyCode.RightArrow;
	public float rewindAdjust = 0f; //Frame fraction

	public float latencySmoothing = 1f; //State delay in seconds
	public SmoothedFloat frameLatency;
	public SmoothedFloat imageLatency;
	public bool overrideDeviceType = false;
	public LeapDeviceType overrideDeviceTypeWith = LeapDeviceType.Invalid;

	protected struct TransformData {
		public long leapTime; // microseconds
		public Vector3 position; //meters
		public Quaternion rotation;

		public static TransformData Lerp(TransformData from, TransformData to, long time) {
			if (from.leapTime == to.leapTime) {
				return from;
			}
			float fraction = (float)(time - from.leapTime) / (float)(to.leapTime - from.leapTime);
			return new TransformData(){
				leapTime = time,
				position = Vector3.Lerp(from.position, to.position, fraction),
				rotation = Quaternion.Slerp(from.rotation, to.rotation, fraction)
			};
		}
	}

	private long timeFrame = 0;
	private long lastFrame = 0;
	private long maxLatency = 200000; //microseconds
	protected List<TransformData> history;

	protected LeapDeviceInfo deviceInfo;

	private Vector3 virtualCameraStereo;

	// Sets default values for this actor's properties
	ACameraWarp();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	virtual TransformData TransformAtTime(long time);
	virtual void Start();
	
	virtual void LateUpdate();
	
	virtual void UpdateAlignment();
	virtual void UpdateTimeWarp();
	virtual bool IsFinite(float f);
	virtual bool IsFinite(Vector3 v);
	virtual bool IsFinite(Quaternion q);
	virtual bool IsFinite(TransformData t);
};
