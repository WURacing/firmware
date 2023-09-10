const styles = {
  container: {
    display: "flex",
    flex: 6,
    flexDirection: "row",
    backgroundColor: "#2B2B2B",
    width: "100vw",
    height: "100vh",
    marginTop: 0,
    marginLeft: 0,
    justifyContent: "center",
  },

  innerContainer: {
    color: "#CC9F07",
    display: "flex",
    flexDirection: "column",
    flex: 5,
    marginLeft: "5%",
    marginRight: "5%",
    width: "90%",
    height: "90%",
    alignSelf: "center",
    backgroundColor: "#2B2B2B",
    borderWidth: 5,
    borderRadius: 10,
    borderStyle: "solid",
    borderColor: "#C8C8C8",
    alignItems: "center",
  },

  rpmContainer: {
    color: "#CC9F07",
    backgroundColor: "red",
    width: "90%",
    height: "12%",
    marginTop: 20,
    borderColor: "#CC9F07",
    borderWidth: 2,
    borderStyle: "solid",
  },

  rpmStyle: {
    backgroundColor: "#2B2B2B",
    width: "93%",
    height: "100%",
  },

  blockTitle: {
    position: "absolute",
    top: -10,
    color: "#C8C8C8",
    fontSize: 24,
  },

  centerDisplay: {
    width: "90%",
    height: "40%",
    marginTop: 25,
    display: "flex",
    flexDirection: "row",
  },

  centerLeft: {
    flex: 1,
    marginLeft: -10,
    display: "flex",
    flexDirection: "row",
    flexWrap: "wrap",
    alignItems: "center",
    justifyContent: "center",
  },

  centerLeftBlock: {
    width: "40%",
    height: "35%",
    position: "relative",
    display: "flex",
    flexDirection: "column",
    textAlign: "center",
    color: "#C8C8C8",
    alignItems: "center",
    fontSize: 60,
    justifyContent: "center",
    marginLeft: 10,
    marginRight: 10,
    textAlign: "center",
    backgroundColor: "#2B2B2B",
    borderColor: "#C8C8C8",
    borderTop: 0,
    borderTopColor: "#2B2B2B",
    borderWidth: 5,
    borderStyle: "solid",
  },

  centerCenter: {
    flex: 1,
    display: "flex",
    flexDirection: "row",
    alignItems: "center",
    justifyContent: "center",
  },

  centerCenterBlock: {
    width: "90%",
    height: "85%",
    display: "flex",
    textAlign: "center",
    color: "#C8C8C8",
    alignItems: "center",
    fontSize: 240,
    justifyContent: "center",
    backgroundColor: "#2B2B2B",
    borderColor: "#C8C8C8",
    borderWidth: 5,
    borderStyle: "solid",
  },

  centerRight: {
    flex: 1,
    marginRight: -20,
    display: "flex",
    flexDirection: "row",
    flexWrap: "wrap",
    alignItems: "center",
    justifyContent: "center",
  },

  centerRightBlock: {
    width: "80%",
    height: "35%",
    marginLeft: 10,
    marginRight: 10,
    position: "relative",
    display: "flex",
    flexDirection: "column",
    textAlign: "center",
    color: "#C8C8C8",
    alignItems: "center",
    fontSize: 60,
    justifyContent: "center",
    backgroundColor: "#2B2B2B",
    borderColor: "#C8C8C8",
    borderTopColor: "#2B2B2B",
    borderWidth: 5,
    borderStyle: "solid",
  },

  bottomDisplay: {
    width: "90%",
    height: "35%",
    marginTop: 25,
    display: "flex",
    flexDirection: "row",
  },

  bottomFirst: {
    flex: 1.4,
    display: "flex",
    flexDirection: "column",
    justifyContent: "center",
    alignItems: "center",
  },

  bottomFirstBlock: {
    width: "90%",
    height: "60%",
    position: "relative",
    marginTop: 10,
    marginBottom: 10,
    display: "flex",
    flexDirection: "column",
    textAlign: "center",
    color: "#C8C8C8",
    alignItems: "center",
    fontSize: 60,
    justifyContent: "center",
    backgroundColor: "#2B2B2B",
    borderColor: "#C8C8C8",
    borderTop: 0,
    borderTopColor: "#2B2B2B",
    borderWidth: 5,
    borderStyle: "solid",
  },

  bottomFirstProgressBox: {
    position: "absolute",
    display: "flex",
    alignItems: "center",
    top: "50%",
    marginTop: "10%",
    width: "50%",
    height: "50%",
    transform: "rotate(-90deg)",
  },

  bottomFirstProgress: {
    width: "100%",
    height: "100%",
  },

  bottomSecond: {
    flex: 1,
    display: "flex",
    flexDirection: "column",
    justifyContent: "center",
    alignItems: "center",
  },

  bottomSecondBlock: {
    position: "relative",
    width: "80%",
    height: "40%",
    marginTop: 10,
    marginBottom: 10,
    display: "flex",
    flexDirection: "column",
    textAlign: "center",
    color: "#C8C8C8",
    alignItems: "center",
    fontSize: 60,
    justifyContent: "center",
    backgroundColor: "#2B2B2B",
    borderColor: "#C8C8C8",
    borderTop: 0,
    borderTopColor: "#2B2B2B",
    borderWidth: 5,
    borderStyle: "solid",
  },

  bottomThird: {
    flex: 1.2,
    display: "flex",
    flexDirection: "column",
    justifyContent: "center",
  },

  bottomUpperImage: {
    width: "80%",
    alignSelf: "center",
  },

  bottomLowerImage: {
    width: "30%",
    backgroundImage: "url(./home.png)",
    alignSelf: "center",
  },

  bottomFourth: {
    flex: 1.2,
    display: "flex",
    flexDirection: "column",
    justifyContent: "center",
    alignItems: "center",
    position: "relative",
  },

  bottomFourthImage: {
    width: "80%",
    position: "absolute",
  },

  gForceDot: {
    width: "10%",
    height: "10%",
    borderRadius: "50%",
    alignSelf: "center",
    backgroundColor: "red",
    position: "relative",
    left: 0,
    bottom: 0,
  },

  bottomFifth: {
    color: "#CC9F07",
    flex: 0.625,
    boxSizing: "border-box",
    display: "flex",
    justifyContent: "center",
    maxWidth: 100,
  },

  bottomFifthTitle: {
    color: "#C8C8C8",
    fontSize: 24,
    position: "absolute",
    bottom: "8%",
  },

  brake: {
    minWidth: 200,
    alignSelf: "center",
    backgroundColor: "#2B2B2B",
    borderColor: "#C8C8C8",
    borderWidth: 3,
    borderStyle: "solid",
  },

  gas: {
    minWidth: 200,
    alignSelf: "center",
    backgroundColor: "#2B2B2B",
    borderColor: "#C8C8C8",
    borderWidth: 3,
    borderStyle: "solid",
  },
};

export default styles;
