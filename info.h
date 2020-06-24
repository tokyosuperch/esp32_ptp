struct clockinfo {
	unsigned char CommunicationTechnology;
	unsigned char ClockUuid[6];
	unsigned char ClockStratum;
	unsigned int PortId;
	unsigned int SequenceId;
	char ClockIdentifier[4];
	int ClockVariance;
	unsigned char Preferred;
	unsigned char IsBoundaryClock;
};
